#include "synx/agent/backup_manager.hpp"
#include "synx/crypto.hpp"
#include "synx/logger.hpp"
#include <chrono>
#include <sstream>
#include <iomanip>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace synx::agent {

namespace fs = std::filesystem;

BackupManager::BackupManager(fs::path backups_dir)
    : backups_dir_(std::move(backups_dir)) {
    std::error_code ec;
    fs::create_directories(backups_dir_, ec);
}

Result<BackupMetadata> BackupManager::create_backup(const std::string& instance_id, const fs::path& instance_dir) {
    std::error_code ec;
    if (!fs::exists(instance_dir, ec)) {
        return Result<BackupMetadata>::Err("Instance directory does not exist: " + instance_dir.string());
    }

    auto now = std::chrono::system_clock::now();
    auto timer = std::chrono::system_clock::to_time_t(now);
    std::tm bt{};
#ifdef _WIN32
    localtime_s(&bt, &timer);
#else
    localtime_r(&timer, &bt);
#endif

    std::ostringstream ss;
    ss << "backup_" << instance_id << "_" << std::put_time(&bt, "%Y%m%d_%H%M%S") << ".zip";
    std::string filename = ss.str();
    fs::path target_zip = backups_dir_ / filename;

    SYNX_LOG_INFO("Backup", "Creating backup for instance [" + instance_id + "] -> " + target_zip.string());

#ifdef _WIN32
    // Use native Windows 11 tar.exe to create zip archive
    std::wstring cmd = L"tar.exe -a -c -f \"" + target_zip.wstring() + L"\" -C \"" + instance_dir.wstring() + L"\" .";

    STARTUPINFOW si{};
    si.cb = sizeof(STARTUPINFOW);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    PROCESS_INFORMATION pi{};
    BOOL ok = CreateProcessW(nullptr, cmd.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
    if (!ok) {
        return Result<BackupMetadata>::Err("Failed to invoke archive utility: " + std::to_string(GetLastError()));
    }

    WaitForSingleObject(pi.hProcess, 60000); // 1 minute timeout
    DWORD exit_code = 0;
    GetExitCodeProcess(pi.hProcess, &exit_code);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    if (exit_code != 0 || !fs::exists(target_zip, ec)) {
        return Result<BackupMetadata>::Err("Archive utility exited with error code: " + std::to_string(exit_code));
    }
#else
    return Result<BackupMetadata>::Err("Unsupported platform for archive creation");
#endif

    BackupMetadata meta{};
    meta.id = generate_uuid();
    meta.instance_id = instance_id;
    meta.filename = filename;
    meta.full_path = target_zip.string();
    meta.size_bytes = fs::file_size(target_zip, ec);
    meta.created_epoch_sec = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();

    // Compute SHA256 checksum
    auto hash_res = Crypto::sha256_file(target_zip.string());
    if (hash_res.is_ok()) {
        meta.sha256_checksum = hash_res.value();
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        backups_[meta.id] = meta;
    }

    SYNX_LOG_INFO("Backup", "Backup completed: " + filename + " (" + std::to_string(meta.size_bytes) + " bytes, SHA256: " + meta.sha256_checksum.substr(0, 8) + "...)");
    return Result<BackupMetadata>::Ok(meta);
}

Result<bool> BackupManager::verify_backup(const std::string& backup_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = backups_.find(backup_id);
    if (it == backups_.end()) {
        return Result<bool>::Err("Backup ID not found: " + backup_id);
    }

    auto hash_res = Crypto::sha256_file(it->second.full_path);
    if (hash_res.is_err()) {
        return Result<bool>::Err(hash_res.error());
    }

    bool matches = (hash_res.value() == it->second.sha256_checksum);
    return Result<bool>::Ok(matches);
}

std::vector<BackupMetadata> BackupManager::list_backups_for_instance(const std::string& instance_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<BackupMetadata> list;
    for (const auto& [_, b] : backups_) {
        if (b.instance_id == instance_id) {
            list.push_back(b);
        }
    }
    return list;
}

Result<void> BackupManager::delete_backup(const std::string& backup_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = backups_.find(backup_id);
    if (it == backups_.end()) {
        return Result<void>::Err("Backup ID not found: " + backup_id);
    }

    std::error_code ec;
    fs::remove(it->second.full_path, ec);
    backups_.erase(it);
    return Result<void>::Ok();
}

} // namespace synx::agent

