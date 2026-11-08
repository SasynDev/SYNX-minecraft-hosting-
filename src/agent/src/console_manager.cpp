#include "synx/agent/console_manager.hpp"

namespace synx::agent {

ConsoleManager::ConsoleManager(size_t max_lines_per_instance)
    : max_lines_(max_lines_per_instance) {}

void ConsoleManager::append_line(const std::string& instance_id, const std::string& line) {
    std::vector<ConsoleListener> listeners_copy;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto& buf = buffers_[instance_id];
        if (buf.size() >= max_lines_) {
            buf.pop_front();
        }
        buf.push_back(line);
        listeners_copy = listeners_;
    }

    for (const auto& listener : listeners_copy) {
        if (listener) {
            listener(instance_id, line);
        }
    }
}

std::vector<std::string> ConsoleManager::get_lines(const std::string& instance_id, size_t max_lines) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = buffers_.find(instance_id);
    if (it == buffers_.end()) {
        return {};
    }

    const auto& buf = it->second;
    size_t count = (std::min)(max_lines, buf.size());
    std::vector<std::string> result;
    result.reserve(count);

    auto start_it = buf.end() - count;
    result.assign(start_it, buf.end());
    return result;
}

void ConsoleManager::clear(const std::string& instance_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    buffers_.erase(instance_id);
}

void ConsoleManager::add_listener(ConsoleListener listener) {
    std::lock_guard<std::mutex> lock(mutex_);
    listeners_.push_back(std::move(listener));
}

void ConsoleManager::clear_listeners() {
    std::lock_guard<std::mutex> lock(mutex_);
    listeners_.clear();
}

} // namespace synx::agent

