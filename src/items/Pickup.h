#pragma once

#include <SFML/Graphics.hpp>

enum class PickupType {
    Coin,
    Heart,
    Key,
    Bomb
};

struct PickupData {
    PickupType type{PickupType::Coin};
    sf::Vector2f position;
    bool collected{false};
};
