#pragma once

#include <SFML/Graphics.hpp>
#include <optional>

#include "../items/Item.h"

class Player;
class Room;

class HUD {
public:
    HUD();
    void draw(sf::RenderTarget& target, const Player& player, sf::Vector2f origin, float scale) const;
    void drawStatsPanel(sf::RenderTarget& target,
                        const Player& player,
                        const std::optional<Item>& recentItem,
                        float recentItemAlpha,
                        bool expanded,
                        sf::Vector2f origin, float scale) const;
    void drawBossBar(sf::RenderTarget& target, const Room& room, sf::Vector2f origin, float scale) const;
    void drawItemPickup(sf::RenderTarget& target, const Item& item, float timeRemaining, sf::Vector2f origin, float scale) const;
    void drawGameOver(sf::RenderTarget& target, sf::Vector2f origin, float scale) const;

private:
    void drawHeart(sf::RenderTarget& target, sf::Vector2f center, sf::Color color, float scale, sf::Vector2f origin, float uiScale) const;
    void drawDigit(sf::RenderTarget& target, sf::Vector2f position, int digit, sf::Color color, float scale, sf::Vector2f origin, float uiScale) const;
    void drawNumber(sf::RenderTarget& target, sf::Vector2f position, int value, sf::Color color, float scale, sf::Vector2f origin, float uiScale) const;
    void drawCoinIcon(sf::RenderTarget& target, sf::Vector2f center, sf::Vector2f origin, float scale) const;
    void drawKeyIcon(sf::RenderTarget& target, sf::Vector2f center, sf::Vector2f origin, float scale) const;
    void drawBombIcon(sf::RenderTarget& target, sf::Vector2f center, sf::Vector2f origin, float scale) const;

    sf::Font m_font;
    bool m_hasFont;
};
