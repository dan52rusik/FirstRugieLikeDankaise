#pragma once

#include <fstream>
#include <initializer_list>
#include <nlohmann/json.hpp>
#include <sstream>
#include <string>

namespace JsonParser {

inline std::string readFile(const std::string& path) {
    std::ifstream input(path);
    if (!input) {
        return {};
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

inline std::string locateFile(std::initializer_list<std::string> candidates) {
    for (const auto& path : candidates) {
        std::ifstream input(path);
        if (input) {
            return path;
        }
    }

    return {};
}

inline nlohmann::json readJson(std::initializer_list<std::string> candidates) {
    const std::string path = locateFile(candidates);
    if (path.empty()) {
        return nlohmann::json();
    }

    std::ifstream input(path);
    if (!input) {
        return nlohmann::json();
    }

    try {
        nlohmann::json json;
        input >> json;
        return json;
    } catch (const nlohmann::json::parse_error&) {
        return nlohmann::json();
    }
}

}
