#pragma once

#include "synx/types.hpp"
#include <string>
#include <vector>

namespace synx {

class Crypto {
public:
    static std::string generate_random_token(size_t length = 32);
    static std::string sha256_string(const std::string& input);
    static Result<std::string> sha256_file(const std::string& filepath);
    static bool constant_time_compare(const std::string& a, const std::string& b);
};

} // namespace synx

