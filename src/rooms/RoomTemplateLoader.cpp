#include "RoomTemplateLoader.h"

#include <algorithm>
#include <cctype>
#include <stdexcept>

#include "../utils/JsonParser.h"

namespace {
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

std::vector<std::string> extractGridLines(const nlohmann::json& entry) {
    std::vector<std::string> lines;
    if (!entry.contains("grid") || !entry["grid"].is_array()) {
        return lines;
    }

    for (const auto& line : entry["grid"]) {
        if (line.is_string()) {
            lines.push_back(line.get<std::string>());
        }
    }
    return lines;
}

RoomTemplate parseTemplate(const nlohmann::json& entry) {
    RoomTemplate result;
    result.id = entry.value("id", "");
    result.roomType = parseRoomType(entry.value("roomType", "normal"));
    result.weight = std::max(1, entry.value("weight", 1));

    const std::vector<std::string> lines = extractGridLines(entry);
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
        const nlohmann::json json = JsonParser::readJson({
            "data/rooms/templates.json",
            "../data/rooms/templates.json",
            "../../data/rooms/templates.json"
        });

        std::vector<RoomTemplate> loaded;
        if (json.contains("templates") && json["templates"].is_array()) {
            for (const auto& entry : json["templates"]) {
                RoomTemplate parsed = parseTemplate(entry);
                if (!parsed.id.empty()) {
                    loaded.push_back(std::move(parsed));
                }
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
