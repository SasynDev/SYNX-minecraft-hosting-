#include "synx/filesystem.hpp"
#include <cassert>
#include <iostream>
#include <filesystem>

int main() {
    std::cout << "[TEST] Running test_filesystem (Sandboxing & Traversal Prevention)..." << std::endl;

    std::filesystem::path test_jail = "temp/test_jail_sandbox";
    std::filesystem::remove_all(test_jail);
    std::filesystem::create_directories(test_jail);

    synx::SandboxedFilesystem fs(test_jail);

    // 1. Safe file operations inside jail
    auto write_res = fs.write_text_file("test.txt", "Hello SYNX Hosting!");
    assert(write_res.is_ok());

    auto read_res = fs.read_text_file("test.txt");
    assert(read_res.is_ok());
    assert(read_res.value() == "Hello SYNX Hosting!");

    // Subdirectory test
    auto mkdir_res = fs.create_directory_safe("plugins");
    assert(mkdir_res.is_ok());

    auto write_sub_res = fs.write_text_file("plugins/config.yml", "enabled: true");
    assert(write_sub_res.is_ok());

    auto list_res = fs.list_directory("");
    assert(list_res.is_ok());
    assert(list_res.value().size() == 2);

    // 2. Traversal Prevention Tests (MUST FAIL)
    auto traversal_1 = fs.resolve_safe_path("../secret.txt");
    assert(traversal_1.is_err());
    std::cout << "  [PASS] Blocked traversal '../secret.txt': " << traversal_1.error() << std::endl;

    auto traversal_2 = fs.resolve_safe_path("plugins/../../escaped.txt");
    assert(traversal_2.is_err());
    std::cout << "  [PASS] Blocked traversal 'plugins/../../escaped.txt': " << traversal_2.error() << std::endl;

    auto traversal_3 = fs.resolve_safe_path("C:\\Windows\\System32\\cmd.exe");
    assert(traversal_3.is_err());
    std::cout << "  [PASS] Blocked absolute drive path: " << traversal_3.error() << std::endl;

    auto traversal_null = fs.resolve_safe_path(std::string("test\0.txt", 9));
    assert(traversal_null.is_err());
    std::cout << "  [PASS] Blocked null byte injection: " << traversal_null.error() << std::endl;

    // 3. Quota calculation
    uint64_t total_size = fs.calculate_total_size_bytes();
    assert(total_size > 0);
    assert(fs.verify_quota(1024 * 1024)); // Under 1MB
    assert(!fs.verify_quota(5));          // Over 5 bytes

    // Cleanup
    std::filesystem::remove_all(test_jail);

    std::cout << "[TEST] test_filesystem PASSED!" << std::endl;
    return 0;
}

