#pragma once

#include <SFML/Graphics.hpp>
#include <optional>

#include "../items/Item.h"

class Player;
class Room;

class HUD {
public:
    HUD();
    void draw(sf::RenderTarget& target, const Player& player) const;
    void drawStatsPanel(sf::RenderTarget& target,
                        const Player& player,
                        const std::optional<Item>& recentItem,
                        float recentItemAlpha,
                        bool expanded) const;
    void drawBossBar(sf::RenderTarget& target, const Room& room) const;
    void drawItemPickup(sf::RenderTarget& target, const Item& item, float timeRemaining) const;
    void drawGameOver(sf::RenderTarget& target) const;

private:
    void drawHeart(sf::RenderTarget& target, sf::Vector2f center, sf::Color color, float scale) const;
    void drawDigit(sf::RenderTarget& target, sf::Vector2f position, int digit, sf::Color color, float scale) const;
    void drawNumber(sf::RenderTarget& target, sf::Vector2f position, int value, sf::Color color, float scale) const;
    void drawCoinIcon(sf::RenderTarget& target, sf::Vector2f center) const;
    void drawKeyIcon(sf::RenderTarget& target, sf::Vector2f center) const;
    void drawBombIcon(sf::RenderTarget& target, sf::Vector2f center) const;

    sf::Font m_font;
    bool m_hasFont;
};
