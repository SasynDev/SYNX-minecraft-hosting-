#include "synx/filesystem.hpp"
#include <fstream>
#include <sstream>
#include <system_error>
#include <algorithm>

namespace synx {

namespace fs = std::filesystem;

SandboxedFilesystem::SandboxedFilesystem(fs::path root_jail) {
    std::error_code ec;
    jail_root_ = fs::absolute(root_jail, ec);
    if (!fs::exists(jail_root_, ec)) {
        fs::create_directories(jail_root_, ec);
    }
    jail_root_ = fs::weakly_canonical(jail_root_, ec);
}

bool SandboxedFilesystem::is_within_jail(const fs::path& resolved_path) const {
    std::error_code ec;
    fs::path norm_target = fs::weakly_canonical(resolved_path, ec);
    fs::path norm_root = jail_root_;

    // Compare paths using lexical relative
    fs::path rel = norm_target.lexically_relative(norm_root);
    if (rel.empty()) return false;

    // If rel starts with ".." or contains drive letter mismatch, it's outside jail
    std::string rel_str = rel.generic_string();
    if (rel_str == ".." || rel_str.rfind("../", 0) == 0 || rel_str.find("/../") != std::string::npos) {
        return false;
    }

    // Double check that norm_target string actually begins with norm_root string
    std::string target_s = norm_target.generic_string();
    std::string root_s = norm_root.generic_string();

    // Ensure case-insensitive or exact prefix comparison
    if (target_s.length() < root_s.length()) return false;

    std::string target_prefix = target_s.substr(0, root_s.length());
    // On Windows, paths are case-insensitive
#ifdef _WIN32
    std::string s1 = target_prefix;
    std::string s2 = root_s;
    std::transform(s1.begin(), s1.end(), s1.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    std::transform(s2.begin(), s2.end(), s2.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    if (s1 != s2) return false;
#else
    if (target_prefix != root_s) return false;
#endif

    // If target has extra characters, the next character must be a path separator
    if (target_s.length() > root_s.length()) {
        char next_ch = target_s[root_s.length()];
        if (next_ch != '/' && next_ch != '\\') return false;
    }

    return true;
}

Result<fs::path> SandboxedFilesystem::resolve_safe_path(const std::string& rel_path) const {
    // Basic sanitization
    if (rel_path.find('\0') != std::string::npos) {
        return Result<fs::path>::Err("Security Error: Null byte detected in path");
    }

    // Disallow absolute drive letters on Windows (e.g. "C:\", "D:/")
    if (rel_path.length() >= 2 && rel_path[1] == ':') {
        return Result<fs::path>::Err("Security Error: Absolute drive path prohibited");
    }

    // Strip leading slashes to prevent root escapes
    std::string cleaned = rel_path;
    while (!cleaned.empty() && (cleaned.front() == '/' || cleaned.front() == '\\')) {
        cleaned.erase(cleaned.begin());
    }

    fs::path candidate = jail_root_ / cleaned;
    if (!is_within_jail(candidate)) {
        return Result<fs::path>::Err("Security Violation: Path escapes sandbox boundary (" + rel_path + ")");
    }

    return Result<fs::path>::Ok(candidate);
}

Result<std::vector<FileItem>> SandboxedFilesystem::list_directory(const std::string& rel_path) const {
    auto resolved = resolve_safe_path(rel_path);
    if (resolved.is_err()) {
        return Result<std::vector<FileItem>>::Err(resolved.error());
    }

    std::error_code ec;
    fs::path target = resolved.value();
    if (!fs::exists(target, ec) || !fs::is_directory(target, ec)) {
        return Result<std::vector<FileItem>>::Err("Directory does not exist or is not a directory: " + rel_path);
    }

    std::vector<FileItem> items;
    for (const auto& entry : fs::directory_iterator(target, ec)) {
        FileItem item;
        item.name = entry.path().filename().string();
        
        fs::path item_rel = entry.path().lexically_relative(jail_root_);
        item.relative_path = item_rel.generic_string();
        item.is_directory = entry.is_directory(ec);
        item.extension = entry.path().extension().string();

        if (!item.is_directory) {
            item.size_bytes = entry.file_size(ec);
        } else {
            item.size_bytes = 0;
        }

        auto ftime = entry.last_write_time(ec);
        auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now()
        );
        item.modified_epoch_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            sctp.time_since_epoch()
        ).count();

        items.push_back(item);
    }

    return Result<std::vector<FileItem>>::Ok(items);
}

Result<void> SandboxedFilesystem::create_directory_safe(const std::string& rel_path) {
    auto resolved = resolve_safe_path(rel_path);
    if (resolved.is_err()) {
        return Result<void>::Err(resolved.error());
    }

    std::error_code ec;
    fs::create_directories(resolved.value(), ec);
    if (ec) {
        return Result<void>::Err("Failed to create directory: " + ec.message());
    }
    return Result<void>::Ok();
}

Result<void> SandboxedFilesystem::delete_path_safe(const std::string& rel_path, bool recursive) {
    auto resolved = resolve_safe_path(rel_path);
    if (resolved.is_err()) {
        return Result<void>::Err(resolved.error());
    }

    // Safety: Do not delete jail_root itself
    if (resolved.value() == jail_root_) {
        return Result<void>::Err("Security Error: Cannot delete sandbox root directory");
    }

    std::error_code ec;
    if (recursive) {
        fs::remove_all(resolved.value(), ec);
    } else {
        fs::remove(resolved.value(), ec);
    }

    if (ec) {
        return Result<void>::Err("Failed to delete path: " + ec.message());
    }
    return Result<void>::Ok();
}

Result<std::string> SandboxedFilesystem::read_text_file(const std::string& rel_path, size_t max_bytes) const {
    auto resolved = resolve_safe_path(rel_path);
    if (resolved.is_err()) {
        return Result<std::string>::Err(resolved.error());
    }

    std::error_code ec;
    fs::path target = resolved.value();
    if (!fs::exists(target, ec) || fs::is_directory(target, ec)) {
        return Result<std::string>::Err("File not found or is a directory: " + rel_path);
    }

    uint64_t file_size = fs::file_size(target, ec);
    if (file_size > max_bytes) {
        return Result<std::string>::Err("File size (" + std::to_string(file_size) + " bytes) exceeds safety maximum (" + std::to_string(max_bytes) + " bytes)");
    }

    std::ifstream file(target, std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        return Result<std::string>::Err("Could not open file: " + rel_path);
    }

    std::ostringstream ss;
    ss << file.rdbuf();
    return Result<std::string>::Ok(ss.str());
}

Result<void> SandboxedFilesystem::write_text_file(const std::string& rel_path, const std::string& content) {
    auto resolved = resolve_safe_path(rel_path);
    if (resolved.is_err()) {
        return Result<void>::Err(resolved.error());
    }

    fs::path target = resolved.value();
    if (target.has_parent_path()) {
        std::error_code ec;
        fs::create_directories(target.parent_path(), ec);
    }

    std::ofstream file(target, std::ios::out | std::ios::binary | std::ios::trunc);
    if (!file.is_open()) {
        return Result<void>::Err("Could not open file for writing: " + rel_path);
    }

    file.write(content.data(), content.size());
    file.flush();
    return Result<void>::Ok();
}

Result<void> SandboxedFilesystem::append_text_file(const std::string& rel_path, const std::string& content) {
    auto resolved = resolve_safe_path(rel_path);
    if (resolved.is_err()) {
        return Result<void>::Err(resolved.error());
    }

    fs::path target = resolved.value();
    std::ofstream file(target, std::ios::out | std::ios::binary | std::ios::app);
    if (!file.is_open()) {
        return Result<void>::Err("Could not open file for append: " + rel_path);
    }

    file.write(content.data(), content.size());
    file.flush();
    return Result<void>::Ok();
}

uint64_t SandboxedFilesystem::calculate_total_size_bytes() const {
    uint64_t total = 0;
    std::error_code ec;
    if (!fs::exists(jail_root_, ec)) return 0;

    for (const auto& entry : fs::recursive_directory_iterator(jail_root_, fs::directory_options::skip_permission_denied, ec)) {
        if (entry.is_regular_file(ec)) {
            total += entry.file_size(ec);
        }
    }
    return total;
}

bool SandboxedFilesystem::verify_quota(uint64_t quota_bytes) const {
    return calculate_total_size_bytes() <= quota_bytes;
}

} // namespace synx
