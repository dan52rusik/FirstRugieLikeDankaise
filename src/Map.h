#pragma once

#include <SFML/Graphics.hpp>

#include "Floor.h"

class Map {
public:
    Map() = default;
    void drawMiniMap(sf::RenderTarget& target, const Floor& floor) const;
};
