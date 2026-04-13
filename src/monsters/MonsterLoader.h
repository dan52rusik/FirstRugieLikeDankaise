#pragma once

#include <string>
#include <vector>
#include <SFML/Graphics.hpp>

struct MonsterData {
    std::string id;
    float hp;
    float speed;
    float damage;
    sf::Color color;
};

class MonsterLoader {
public:
    static const MonsterData& get(const std::string& id);
};
