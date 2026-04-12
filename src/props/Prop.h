#pragma once

#include <SFML/Graphics.hpp>

enum class PropType {
    Barrel,
    Rock,
    Poop,
    Spike,
    Fire,
    Chest
};

struct PropData {
    PropType type{PropType::Barrel};
    sf::Vector2f position;
    bool destroyed{false};
};
