#pragma once

#include "synx/types.hpp"
#include <filesystem>
#include <string>
#include <vector>
#include <cstdint>

namespace synx {

struct FileItem {
    std::string name;
    std::string relative_path;
    bool is_directory{false};
    uint64_t size_bytes{0};
    uint64_t modified_epoch_ms{0};
    std::string extension;
};

class SandboxedFilesystem {
public:
    explicit SandboxedFilesystem(std::filesystem::path root_jail);

    const std::filesystem::path& jail_root() const { return jail_root_; }

    // Resolve and validate a relative path, strictly ensuring it cannot escape jail_root
    Result<std::filesystem::path> resolve_safe_path(const std::string& rel_path) const;

    // Checks if target is contained within the root jail
    bool is_within_jail(const std::filesystem::path& resolved_path) const;

    // Directory operations
    Result<std::vector<FileItem>> list_directory(const std::string& rel_path = "") const;
    Result<void> create_directory_safe(const std::string& rel_path);
    Result<void> delete_path_safe(const std::string& rel_path, bool recursive = false);

    // File content operations
    Result<std::string> read_text_file(const std::string& rel_path, size_t max_bytes = 10 * 1024 * 1024) const;
    Result<void> write_text_file(const std::string& rel_path, const std::string& content);
    Result<void> append_text_file(const std::string& rel_path, const std::string& content);

    // Disk space quota calculation
    uint64_t calculate_total_size_bytes() const;
    bool verify_quota(uint64_t quota_bytes) const;

private:
    std::filesystem::path jail_root_;
};

} // namespace synx

