#pragma once

#include <SFML/Graphics.hpp>

class Player;

class HUD {
public:
    HUD();
    void draw(sf::RenderTarget& target, const Player& player) const;

private:
    sf::Font m_font;
    bool m_hasFont;
};
