#include "RoomTemplateLoader.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <regex>
#include <sstream>
#include <stdexcept>

namespace {
std::string readFile(const std::string& path) {
    std::ifstream input(path);
    if (!input) {
        return {};
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

std::string locateTemplateFile() {
    static const std::vector<std::string> candidates{
        "data/rooms/templates.json",
        "../data/rooms/templates.json",
        "../../data/rooms/templates.json"
    };

    for (const auto& path : candidates) {
        std::ifstream input(path);
        if (input) {
            return path;
        }
    }
    return {};
}

std::string trim(const std::string& value) {
    std::size_t begin = 0;
    std::size_t end = value.size();
    while (begin < end && std::isspace(static_cast<unsigned char>(value[begin]))) {
        ++begin;
    }
    while (end > begin && std::isspace(static_cast<unsigned char>(value[end - 1]))) {
        --end;
    }
    return value.substr(begin, end - begin);
}

std::string extractStringField(const std::string& object, const std::string& field) {
    const std::regex pattern("\"" + field + "\"\\s*:\\s*\"([^\"]*)\"");
    std::smatch match;
    if (std::regex_search(object, match, pattern)) {
        return match[1].str();
    }
    return {};
}

int extractIntField(const std::string& object, const std::string& field, int defaultValue) {
    const std::regex pattern("\"" + field + "\"\\s*:\\s*(-?\\d+)");
    std::smatch match;
    if (std::regex_search(object, match, pattern)) {
        return std::stoi(match[1].str());
    }
    return defaultValue;
}

RoomType parseRoomType(const std::string& value) {
    if (value == "start") {
        return RoomType::Start;
    }
    if (value == "treasure") {
        return RoomType::Treasure;
    }
    if (value == "boss") {
        return RoomType::Boss;
    }
    return RoomType::Normal;
}

std::vector<std::string> extractGridLines(const std::string& object) {
    const std::regex gridPattern("\"grid\"\\s*:\\s*\\[([\\s\\S]*?)\\]");
    const std::regex linePattern("\"([^\"]*)\"");
    std::smatch gridMatch;
    std::vector<std::string> lines;
    if (!std::regex_search(object, gridMatch, gridPattern)) {
        return lines;
    }

    const std::string gridBody = gridMatch[1].str();
    auto begin = std::sregex_iterator(gridBody.begin(), gridBody.end(), linePattern);
    auto end = std::sregex_iterator();
    for (auto it = begin; it != end; ++it) {
        lines.push_back((*it)[1].str());
    }
    return lines;
}

RoomTemplate parseTemplate(const std::string& object) {
    RoomTemplate result;
    result.id = extractStringField(object, "id");
    result.roomType = parseRoomType(extractStringField(object, "roomType"));
    result.weight = std::max(1, extractIntField(object, "weight", 1));

    const std::vector<std::string> lines = extractGridLines(object);
    for (std::size_t row = 0; row < lines.size(); ++row) {
        const std::string line = trim(lines[row]);
        for (std::size_t col = 0; col < line.size(); ++col) {
            const sf::Vector2i tile(static_cast<int>(col), static_cast<int>(row));
            switch (line[col]) {
            case '#':
                result.rocks.push_back(tile);
                break;
            case 'b':
                result.barrels.push_back(tile);
                break;
            case 'm':
                result.monsterSpawns.push_back(tile);
                break;
            case 'r':
                result.rewardTile = tile;
                break;
            case 'x':
                result.blockedTiles.push_back(tile);
                break;
            default:
                break;
            }
        }
    }

    return result;
}

std::vector<std::string> extractObjects(const std::string& json) {
    std::vector<std::string> objects;
    const std::size_t templatesPos = json.find("\"templates\"");
    if (templatesPos == std::string::npos) {
        return objects;
    }

    const std::size_t arrayStart = json.find('[', templatesPos);
    if (arrayStart == std::string::npos) {
        return objects;
    }

    int depth = 0;
    std::size_t objectStart = std::string::npos;
    for (std::size_t i = arrayStart; i < json.size(); ++i) {
        if (json[i] == '{') {
            if (depth == 0) {
                objectStart = i;
            }
            ++depth;
        } else if (json[i] == '}') {
            --depth;
            if (depth == 0 && objectStart != std::string::npos) {
                objects.push_back(json.substr(objectStart, i - objectStart + 1));
                objectStart = std::string::npos;
            }
        }
    }
    return objects;
}

std::vector<RoomTemplate> fallbackTemplates() {
    return {
        {"normal_cross", RoomType::Normal, 1, {{2, 2}, {12, 2}, {2, 6}, {12, 6}}, {{7, 2}, {4, 4}, {10, 4}}, {{3, 2}, {7, 2}, {11, 2}, {3, 6}, {7, 6}, {11, 6}}, {}, std::nullopt},
        {"normal_box", RoomType::Normal, 1, {{4, 2}, {10, 2}, {4, 6}, {10, 6}}, {{7, 1}, {3, 4}, {11, 4}}, {{2, 2}, {7, 2}, {12, 2}, {2, 6}, {7, 6}, {12, 6}}, {}, std::nullopt},
        {"treasure_center", RoomType::Treasure, 1, {{4, 2}, {10, 2}, {4, 6}, {10, 6}}, {}, {}, {}, sf::Vector2i(7, 4)},
        {"boss_default", RoomType::Boss, 1, {{4, 3}, {10, 3}, {7, 6}}, {}, {}, {}, std::nullopt},
        {"start_plain", RoomType::Start, 1, {{7, 4}}, {}, {}, {}, std::nullopt}
    };
}
}

const std::vector<RoomTemplate>& RoomTemplateLoader::loadAll() {
    static const std::vector<RoomTemplate> templates = [] {
        const std::string path = locateTemplateFile();
        if (path.empty()) {
            return fallbackTemplates();
        }

        const std::string json = readFile(path);
        std::vector<RoomTemplate> loaded;
        for (const auto& object : extractObjects(json)) {
            RoomTemplate parsed = parseTemplate(object);
            if (!parsed.id.empty()) {
                loaded.push_back(std::move(parsed));
            }
        }

        if (loaded.empty()) {
            return fallbackTemplates();
        }
        return loaded;
    }();

    return templates;
}

const RoomTemplate& RoomTemplateLoader::pick(RoomType roomType, int seed) {
    const auto& templates = loadAll();

    std::vector<const RoomTemplate*> matching;
    int totalWeight = 0;
    for (const auto& roomTemplate : templates) {
        if (roomTemplate.roomType != roomType) {
            continue;
        }
        matching.push_back(&roomTemplate);
        totalWeight += roomTemplate.weight;
    }

    if (matching.empty()) {
        for (const auto& roomTemplate : templates) {
            if (roomTemplate.roomType == RoomType::Normal) {
                return roomTemplate;
            }
        }
        throw std::runtime_error("No room templates available");
    }

    int roll = std::abs(seed) % std::max(1, totalWeight);
    for (const RoomTemplate* roomTemplate : matching) {
        if (roll < roomTemplate->weight) {
            return *roomTemplate;
        }
        roll -= roomTemplate->weight;
    }
    return *matching.back();
}
