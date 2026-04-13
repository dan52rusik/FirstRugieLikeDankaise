#pragma once

#include <string>
#include <vector>
#include <regex>
#include <fstream>
#include <sstream>

namespace JsonParser {

inline std::string readFile(const std::string& path) {
    std::ifstream input(path);
    if (!input) return {};
    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

inline std::string extractStringField(const std::string& object, const std::string& field) {
    const std::regex pattern("\"" + field + "\"\\s*:\\s*\"([^\"]*)\"");
    std::smatch match;
    if (std::regex_search(object, match, pattern)) {
        return match[1].str();
    }
    return {};
}

inline float extractFloatField(const std::string& object, const std::string& field, float defaultValue) {
    const std::regex pattern("\"" + field + "\"\\s*:\\s*(-?\\d+\\.?\\d*)");
    std::smatch match;
    if (std::regex_search(object, match, pattern)) {
        return std::stof(match[1].str());
    }
    return defaultValue;
}

inline std::vector<std::string> extractObjects(const std::string& json, const std::string& arrayName) {
    std::vector<std::string> objects;
    const std::size_t arrayPos = json.find("\"" + arrayName + "\"");
    if (arrayPos == std::string::npos) return objects;

    const std::size_t arrayStart = json.find('[', arrayPos);
    if (arrayStart == std::string::npos) return objects;

    int depth = 0;
    std::size_t objectStart = std::string::npos;
    for (std::size_t i = arrayStart; i < json.size(); ++i) {
        if (json[i] == '{') {
            if (depth == 0) objectStart = i;
            ++depth;
        } else if (json[i] == '}') {
            --depth;
            if (depth == 0 && objectStart != std::string::npos) {
                objects.push_back(json.substr(objectStart, i - objectStart + 1));
                objectStart = std::string::npos;
            }
        } else if (json[i] == ']') {
            if (depth == 0) break;
        }
    }
    return objects;
}

}
