#pragma once

#include "synx/types.hpp"
#include <string>
#include <vector>
#include <filesystem>
#include <mutex>

namespace synx::agent {

struct BackupMetadata {
    std::string id;
    std::string instance_id;
    std::string filename;
    std::string full_path;
    uint64_t size_bytes{0};
    std::string sha256_checksum;
    uint64_t created_epoch_sec{0};
};

class BackupManager {
public:
    explicit BackupManager(std::filesystem::path backups_dir);

    // Create a standalone backup archive of an instance directory
    Result<BackupMetadata> create_backup(const std::string& instance_id, const std::filesystem::path& instance_dir);

    // Verify backup integrity via SHA-256 checksum
    Result<bool> verify_backup(const std::string& backup_id) const;

    // List all backups for an instance
    std::vector<BackupMetadata> list_backups_for_instance(const std::string& instance_id) const;

    // Delete a specific backup
    Result<void> delete_backup(const std::string& backup_id);

private:
    std::filesystem::path backups_dir_;
    mutable std::mutex mutex_;
    std::unordered_map<std::string, BackupMetadata> backups_;
};

} // namespace synx::agent

