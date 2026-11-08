#pragma once

#include "synx/types.hpp"
#include <string>
#include <vector>
#include <filesystem>
#include <functional>

namespace synx::agent {

using ProgressCallback = std::function<void(const std::string& status, int percent)>;

class ServerInstaller {
public:
    explicit ServerInstaller(std::filesystem::path cache_dir = "cache");

    // Install server software into target instance directory
    Result<void> install_server(
        const std::string& instance_dir,
        ServerType server_type,
        const std::string& version,
        bool crossplay_enabled = false,
        bool cracked_enabled = true,
        ProgressCallback on_progress = nullptr
    );

    // Download file from URL to local path using WinINet / WinHTTP
    static Result<void> download_file(const std::string& url, const std::filesystem::path& dest_path, ProgressCallback on_progress = nullptr);

    // Setup GeyserMC and Floodgate plugins for Crossplay
    Result<void> install_crossplay_plugins(const std::string& instance_dir, uint16_t bedrock_port);

    // Setup SkinsRestorer plugin for Cracked mode
    Result<void> install_cracked_plugins(const std::string& instance_dir);

private:
    std::filesystem::path cache_dir_;
    std::filesystem::path jars_dir_;
    std::filesystem::path plugins_dir_;
};

} // namespace synx::agent

