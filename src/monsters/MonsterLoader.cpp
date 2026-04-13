#include "MonsterLoader.h"
#include "../utils/JsonParser.h"
#include <map>
#include <regex>
#include <cstdint>

namespace {
MonsterData fallback(const std::string& id) {
    if (id == "fly") return {"fly", 10.0f, 95.0f, 2.0f, sf::Color(70, 70, 70)};
    if (id == "spider") return {"spider", 15.0f, 180.0f, 2.0f, sf::Color(130, 90, 70)};
    if (id == "knight") return {"knight", 40.0f, 120.0f, 2.0f, sf::Color(110, 110, 160)};
    if (id == "leech") return {"leech", 20.0f, 80.0f, 2.0f, sf::Color(120, 20, 20)};
    if (id == "boss") return {"boss", 300.0f, 90.0f, 4.0f, sf::Color(180, 40, 40)};
    return {"unknown", 10.0f, 50.0f, 1.0f, sf::Color::White};
}

sf::Color parseColor(const std::string& obj) {
    const std::regex pattern("\"color\"\\s*:\\s*\\[(\\d+),\\s*(\\d+),\\s*(\\d+)\\]");
    std::smatch match;
    if (std::regex_search(obj, match, pattern)) {
        return sf::Color(
            static_cast<std::uint8_t>(std::stoi(match[1])),
            static_cast<std::uint8_t>(std::stoi(match[2])),
            static_cast<std::uint8_t>(std::stoi(match[3]))
        );
    }
    return sf::Color::White;
}
}

const MonsterData& MonsterLoader::get(const std::string& id) {
    static const std::map<std::string, MonsterData> monsters = [] {
        std::string json = JsonParser::readFile("data/monsters.json");
        if (json.empty()) json = JsonParser::readFile("../data/monsters.json");

        std::map<std::string, MonsterData> loaded;
        if (!json.empty()) {
            for (const auto& obj : JsonParser::extractObjects(json, "monsters")) {
                MonsterData d;
                d.id = JsonParser::extractStringField(obj, "id");
                d.hp = JsonParser::extractFloatField(obj, "hp", 10.0f);
                d.speed = JsonParser::extractFloatField(obj, "speed", 50.0f);
                d.damage = JsonParser::extractFloatField(obj, "damage", 1.0f);
                d.color = parseColor(obj);
                loaded[d.id] = d;
            }
        }
        return loaded;
    }();

    auto it = monsters.find(id);
    if (it != monsters.end()) {
        return it->second;
    }
    
    static std::map<std::string, MonsterData> cache;
    if (cache.find(id) == cache.end()) {
        cache[id] = fallback(id);
    }
    return cache[id];
}
