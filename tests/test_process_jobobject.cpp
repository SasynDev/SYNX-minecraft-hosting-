#include "synx/process.hpp"
#include <cassert>
#include <iostream>
#include <vector>
#include <chrono>

int main() {
    std::cout << "[TEST] Running test_process_jobobject..." << std::endl;

    synx::ManagedProcess proc;
    synx::ResourceLimits limits;
    limits.ram_limit_mb = 1024;
    limits.cpu_limit_pct = 100;

    std::vector<std::string> output_lines;
    auto on_stdout = [&](const std::string& line) {
        output_lines.push_back(line);
    };

    std::string exec = "cmd.exe";
    std::vector<std::string> args = {"/c", "echo", "SYNX_PROCESS_TEST_OUTPUT"};

    auto start_res = proc.start(exec, args, ".", limits, on_stdout);
    assert(start_res.is_ok());

    int wait_cycles = 0;
    while (proc.is_running() && wait_cycles < 20) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        wait_cycles++;
    }

    assert(!proc.is_running());
    assert(proc.exit_code() == 0);

    bool found = false;
    for (const auto& line : output_lines) {
        if (line.find("SYNX_PROCESS_TEST_OUTPUT") != std::string::npos) {
            found = true;
            break;
        }
    }
    assert(found);

    std::cout << "[TEST] test_process_jobobject PASSED!" << std::endl;
    return 0;
}

