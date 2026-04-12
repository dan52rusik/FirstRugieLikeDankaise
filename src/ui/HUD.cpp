#include "HUD.h"

#include "../Player.h"

#include <string>

HUD::HUD() : m_hasFont(m_font.openFromFile("assets/fonts/isaac.ttf")) {}

void HUD::draw(sf::RenderTarget& target, const Player& player) const {
    const int hearts = (player.getMaxHp() + 1) / 2;
    const int currentHp = player.getHp();

    for (int i = 0; i < hearts; ++i) {
        sf::RectangleShape slot({20.0f, 18.0f});
        slot.setPosition({24.0f + i * 24.0f, 664.0f});
        slot.setFillColor(sf::Color(70, 30, 30));

        const int hpForHeart = currentHp - i * 2;
        if (hpForHeart >= 2) {
            slot.setFillColor(sf::Color(210, 45, 55));
        } else if (hpForHeart == 1) {
            slot.setFillColor(sf::Color(230, 120, 120));
        }
        target.draw(slot);
    }

    if (!m_hasFont) {
        return;
    }

    sf::Text text(m_font);
    text.setCharacterSize(20);
    text.setFillColor(sf::Color::White);
    text.setPosition({24.0f, 632.0f});
    text.setString("$ " + std::to_string(player.getCoins()) +
                   "   K " + std::to_string(player.getKeys()) +
                   "   B " + std::to_string(player.getBombs()));
    target.draw(text);
}
