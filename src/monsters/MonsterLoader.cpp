#include "MonsterLoader.h"

#include "../utils/JsonParser.h"

#include <cstdint>
#include <map>

namespace {
MonsterData fallback(const std::string& id) {
    if (id == "fly") return {"fly", 10.0f, 95.0f, 2.0f, sf::Color(70, 70, 70)};
    if (id == "spider") return {"spider", 15.0f, 180.0f, 2.0f, sf::Color(130, 90, 70)};
    if (id == "knight") return {"knight", 40.0f, 120.0f, 2.0f, sf::Color(110, 110, 160)};
    if (id == "leech") return {"leech", 20.0f, 80.0f, 2.0f, sf::Color(120, 20, 20)};
    if (id == "boss") return {"boss", 300.0f, 90.0f, 4.0f, sf::Color(180, 40, 40)};
    return {"unknown", 10.0f, 50.0f, 1.0f, sf::Color::White};
}

sf::Color parseColor(const nlohmann::json& color) {
    if (!color.is_array() || color.size() < 3) {
        return sf::Color::White;
    }

    return sf::Color(
        static_cast<std::uint8_t>(color[0].get<int>()),
        static_cast<std::uint8_t>(color[1].get<int>()),
        static_cast<std::uint8_t>(color[2].get<int>())
    );
}
}

const MonsterData& MonsterLoader::get(const std::string& id) {
    static const std::map<std::string, MonsterData> monsters = [] {
        std::map<std::string, MonsterData> loaded;
        const nlohmann::json json = JsonParser::readJson({
            "data/monsters.json",
            "../data/monsters.json",
            "../../data/monsters.json"
        });

        if (json.contains("monsters") && json["monsters"].is_array()) {
            for (const auto& entry : json["monsters"]) {
                MonsterData data;
                data.id = entry.value("id", "");
                if (data.id.empty()) {
                    continue;
                }

                data.hp = entry.value("hp", 10.0f);
                data.speed = entry.value("speed", 50.0f);
                data.damage = entry.value("damage", 1.0f);
                data.color = parseColor(entry.value("color", nlohmann::json::array()));
                loaded[data.id] = data;
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
