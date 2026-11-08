#pragma once

#include "synx/types.hpp"
#include <string>
#include <vector>
#include <deque>
#include <unordered_map>
#include <mutex>
#include <functional>

namespace synx::agent {

using ConsoleListener = std::function<void(const std::string& instance_id, const std::string& line)>;

class ConsoleManager {
public:
    explicit ConsoleManager(size_t max_lines_per_instance = 1000);

    void append_line(const std::string& instance_id, const std::string& line);
    std::vector<std::string> get_lines(const std::string& instance_id, size_t max_lines = 100) const;
    void clear(const std::string& instance_id);

    void add_listener(ConsoleListener listener);
    void clear_listeners();

private:
    size_t max_lines_;
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::deque<std::string>> buffers_;
    std::vector<ConsoleListener> listeners_;
};

} // namespace synx::agent

