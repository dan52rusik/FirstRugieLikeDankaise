#pragma once

#include <string>
#include <vector>

enum class ItemEffect {
    TearRate,
    Damage,
    Speed
};

struct Item {
    std::string name;
    std::string description;
    ItemEffect effect{ItemEffect::Damage};
    float amount{0.0f};
};

std::vector<Item> createDefaultItems();
