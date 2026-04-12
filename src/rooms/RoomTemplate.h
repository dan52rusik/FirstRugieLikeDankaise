#pragma once

#include <SFML/Graphics.hpp>
#include <optional>
#include <string>
#include <vector>

#include "../Floor.h"

struct RoomTemplate {
    std::string id;
    RoomType roomType{RoomType::Normal};
    int weight{1};
    std::vector<sf::Vector2i> rocks;
    std::vector<sf::Vector2i> barrels;
    std::vector<sf::Vector2i> monsterSpawns;
    std::vector<sf::Vector2i> blockedTiles;
    std::optional<sf::Vector2i> rewardTile;
};
