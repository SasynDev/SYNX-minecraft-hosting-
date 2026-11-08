#pragma once

#include "synx/types.hpp"
#include <string>
#include <vector>
#include <unordered_map>

namespace synx {

struct PropertyLine {
    bool is_comment{false};
    std::string raw_line;
    std::string key;
    std::string value;
};

class ServerProperties {
public:
    ServerProperties() = default;

    static ServerProperties parse(const std::string& content);
    static ServerProperties create_default(const std::string& server_name, uint16_t port, bool cracked_enabled = true);

    std::string to_string() const;

    // Property accessors
    std::string get(const std::string& key, const std::string& default_val = "") const;
    void set(const std::string& key, const std::string& value);
    bool has(const std::string& key) const;

    // Cracked / Offline mode helpers
    bool is_cracked() const {
        return get("online-mode", "true") == "false";
    }

    void set_cracked(bool enabled) {
        set("online-mode", enabled ? "false" : "true");
    }

    // Direct key-value map for API consumption
    std::unordered_map<std::string, std::string> to_map() const;
    void update_from_map(const std::unordered_map<std::string, std::string>& updates);

private:
    std::vector<PropertyLine> lines_;
};

} // namespace synx

