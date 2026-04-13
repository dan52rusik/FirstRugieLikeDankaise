#include "Item.h"
#include "../utils/JsonParser.h"
#include <vector>

namespace {
ItemEffect parseEffect(const std::string& value) {
    if (value == "TearRate") return ItemEffect::TearRate;
    if (value == "Damage") return ItemEffect::Damage;
    if (value == "Speed") return ItemEffect::Speed;
    return ItemEffect::Damage;
}

std::vector<Item> fallbackItems() {
    return {
        {"Sad Onion", "Tears up", ItemEffect::TearRate, -0.06f},
        {"Wire Coat Hanger", "Damage up", ItemEffect::Damage, 1.0f},
        {"Skateboard", "Speed up", ItemEffect::Speed, 24.0f}
    };
}
}

std::vector<Item> createDefaultItems() {
    static const std::vector<Item> items = [] {
        std::string json = JsonParser::readFile("data/items.json");
        if (json.empty()) json = JsonParser::readFile("../data/items.json");
        
        if (json.empty()) return fallbackItems();

        std::vector<Item> loaded;
        for (const auto& obj : JsonParser::extractObjects(json, "items")) {
            Item item;
            item.name = JsonParser::extractStringField(obj, "name");
            item.description = JsonParser::extractStringField(obj, "description");
            item.effect = parseEffect(JsonParser::extractStringField(obj, "effect"));
            item.amount = JsonParser::extractFloatField(obj, "amount", 0.0f);
            loaded.push_back(item);
        }
        return loaded.empty() ? fallbackItems() : loaded;
    }();

    return items;
}
