#pragma once
#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string>
namespace bookmgmt {
inline bool validId(const std::string& id) {
    return std::any_of(id.begin(), id.end(), [](unsigned char c) { return !std::isspace(c); });
}
inline void requireId(const std::string& id, const char* label = "id") {
    if (!validId(id)) throw std::invalid_argument(std::string(label) + " must not be blank");
}
}
