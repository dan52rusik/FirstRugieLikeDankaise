#include "HUD.h"

#include "../Player.h"
#include "../Room.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace {
std::string formatSignedFloat(float value, int decimals) {
    const bool positive = value >= 0.0f;
    const float absValue = std::abs(value);
    const float scale = std::pow(10.0f, static_cast<float>(decimals));
    const int scaled = static_cast<int>(std::round(absValue * scale));
    const int whole = scaled / static_cast<int>(scale);
    const int frac = scaled % static_cast<int>(scale);

    std::string text = positive ? "+" : "-";
    text += std::to_string(whole);
    if (decimals > 0) {
        text += ".";
        std::string fracText = std::to_string(frac);
        while (static_cast<int>(fracText.size()) < decimals) {
            fracText = "0" + fracText;
        }
        text += fracText;
    }
    return text;
}

std::string itemDeltaText(const Item& item) {
    switch (item.effect) {
    case ItemEffect::TearRate:
        return formatSignedFloat(item.amount, 2) + " tear delay";
    case ItemEffect::Damage:
        return formatSignedFloat(item.amount, 1) + " damage";
    case ItemEffect::Speed:
        return formatSignedFloat(item.amount, 0) + " speed";
    }
    return {};
}
}

HUD::HUD() : m_hasFont(false) {
    static const char* kFontPaths[] = {
        "assets/fonts/isaac.ttf",
        "C:/Windows/Fonts/trebuc.ttf",
        "C:/Windows/Fonts/arial.ttf",
        "C:/Windows/Fonts/segoeui.ttf"
    };

    for (const char* path : kFontPaths) {
        if (m_font.loadFromFile(path)) {
            m_hasFont = true;
            break;
        }
    }
}

void HUD::drawDigit(sf::RenderTarget& target, sf::Vector2f position, int digit, sf::Color color, float scale) const {
    static const bool kSegments[10][7] = {
        {true, true, true, false, true, true, true},
        {false, false, true, false, false, true, false},
        {true, false, true, true, true, false, true},
        {true, false, true, true, false, true, true},
        {false, true, true, true, false, true, false},
        {true, true, false, true, false, true, true},
        {true, true, false, true, true, true, true},
        {true, false, true, false, false, true, false},
        {true, true, true, true, true, true, true},
        {true, true, true, true, false, true, true}
    };

    if (digit < 0 || digit > 9) {
        return;
    }

    const float w = 10.0f * scale;
    const float h = 18.0f * scale;
    const float t = 2.0f * scale;

    auto drawSegment = [&](sf::Vector2f segmentPos, sf::Vector2f size) {
        sf::RectangleShape rect(size);
        rect.setPosition(segmentPos);
        rect.setFillColor(color);
        target.draw(rect);
    };

    if (kSegments[digit][0]) {
        drawSegment({position.x, position.y}, {w, t});
    }
    if (kSegments[digit][1]) {
        drawSegment({position.x, position.y}, {t, h * 0.5f});
    }
    if (kSegments[digit][2]) {
        drawSegment({position.x + w - t, position.y}, {t, h * 0.5f});
    }
    if (kSegments[digit][3]) {
        drawSegment({position.x, position.y + h * 0.5f - t * 0.5f}, {w, t});
    }
    if (kSegments[digit][4]) {
        drawSegment({position.x, position.y + h * 0.5f}, {t, h * 0.5f});
    }
    if (kSegments[digit][5]) {
        drawSegment({position.x + w - t, position.y + h * 0.5f}, {t, h * 0.5f});
    }
    if (kSegments[digit][6]) {
        drawSegment({position.x, position.y + h - t}, {w, t});
    }
}

void HUD::drawNumber(sf::RenderTarget& target, sf::Vector2f position, int value, sf::Color color, float scale) const {
    const std::string digits = std::to_string(std::max(0, value));
    for (std::size_t i = 0; i < digits.size(); ++i) {
        drawDigit(target, {position.x + static_cast<float>(i) * 13.0f * scale, position.y}, digits[i] - '0', color, scale);
    }
}

void HUD::drawCoinIcon(sf::RenderTarget& target, sf::Vector2f center) const {
    sf::CircleShape coin(6.0f);
    coin.setOrigin({6.0f, 6.0f});
    coin.setPosition(center);
    coin.setFillColor(sf::Color(236, 204, 82));
    target.draw(coin);

    sf::RectangleShape shine({4.0f, 2.0f});
    shine.setPosition({center.x - 1.0f, center.y - 4.0f});
    shine.setFillColor(sf::Color(255, 236, 154));
    target.draw(shine);
}

void HUD::drawKeyIcon(sf::RenderTarget& target, sf::Vector2f center) const {
    sf::CircleShape ring(4.0f);
    ring.setOrigin({4.0f, 4.0f});
    ring.setPosition({center.x - 4.0f, center.y});
    ring.setFillColor(sf::Color::Transparent);
    ring.setOutlineThickness(2.0f);
    ring.setOutlineColor(sf::Color(210, 210, 210));
    target.draw(ring);

    sf::RectangleShape shaft({11.0f, 2.0f});
    shaft.setPosition({center.x - 1.0f, center.y - 1.0f});
    shaft.setFillColor(sf::Color(210, 210, 210));
    target.draw(shaft);

    sf::RectangleShape toothA({2.0f, 4.0f});
    toothA.setPosition({center.x + 6.0f, center.y - 1.0f});
    toothA.setFillColor(sf::Color(210, 210, 210));
    target.draw(toothA);

    sf::RectangleShape toothB({2.0f, 3.0f});
    toothB.setPosition({center.x + 10.0f, center.y - 1.0f});
    toothB.setFillColor(sf::Color(210, 210, 210));
    target.draw(toothB);
}

void HUD::drawBombIcon(sf::RenderTarget& target, sf::Vector2f center) const {
    sf::CircleShape bomb(6.0f);
    bomb.setOrigin({6.0f, 6.0f});
    bomb.setPosition(center);
    bomb.setFillColor(sf::Color(54, 54, 58));
    target.draw(bomb);

    sf::RectangleShape fuse({5.0f, 2.0f});
    fuse.setPosition({center.x + 2.0f, center.y - 7.0f});
    fuse.setRotation(-35.0f);
    fuse.setFillColor(sf::Color(184, 132, 72));
    target.draw(fuse);

    sf::CircleShape spark(2.0f);
    spark.setOrigin({2.0f, 2.0f});
    spark.setPosition({center.x + 7.0f, center.y - 9.0f});
    spark.setFillColor(sf::Color(250, 210, 94));
    target.draw(spark);
}

void HUD::drawHeart(sf::RenderTarget& target, sf::Vector2f center, sf::Color color, float scale) const {
    sf::CircleShape leftLobe(7.0f * scale);
    leftLobe.setOrigin({7.0f * scale, 7.0f * scale});
    leftLobe.setPosition({center.x - 5.0f * scale, center.y - 5.0f * scale});
    leftLobe.setFillColor(color);

    sf::CircleShape rightLobe(7.0f * scale);
    rightLobe.setOrigin({7.0f * scale, 7.0f * scale});
    rightLobe.setPosition({center.x + 5.0f * scale, center.y - 5.0f * scale});
    rightLobe.setFillColor(color);

    sf::ConvexShape tip(3);
    tip.setPoint(0, {center.x - 13.0f * scale, center.y - 1.0f * scale});
    tip.setPoint(1, {center.x + 13.0f * scale, center.y - 1.0f * scale});
    tip.setPoint(2, {center.x, center.y + 16.0f * scale});
    tip.setFillColor(color);

    target.draw(leftLobe);
    target.draw(rightLobe);
    target.draw(tip);
}

void HUD::draw(sf::RenderTarget& target, const Player& player) const {
    const int hearts = (player.getMaxHp() + 1) / 2;
    const int currentHp = player.getHp();

    sf::RectangleShape heartPanel({122.0f, 48.0f});
    heartPanel.setPosition({14.0f, 658.0f});
    heartPanel.setFillColor(sf::Color(18, 12, 10, 180));
    heartPanel.setOutlineColor(sf::Color(88, 64, 52, 220));
    heartPanel.setOutlineThickness(2.0f);
    target.draw(heartPanel);

    for (int i = 0; i < hearts; ++i) {
        const float x = 34.0f + static_cast<float>(i) * 30.0f;
        const float y = 682.0f;
        const sf::Vector2f center(x, y);

        drawHeart(target, center, sf::Color(55, 22, 24), 0.95f);

        const int hpForHeart = currentHp - i * 2;
        if (hpForHeart >= 2) {
            drawHeart(target, center, sf::Color(215, 42, 60), 0.85f);
        } else if (hpForHeart == 1) {
            drawHeart(target, center, sf::Color(238, 144, 152), 0.85f);
        }
    }

    sf::RectangleShape resourcePanel({230.0f, 44.0f});
    resourcePanel.setPosition({18.0f, 18.0f});
    resourcePanel.setFillColor(sf::Color(18, 12, 10, 176));
    resourcePanel.setOutlineColor(sf::Color(88, 64, 52, 220));
    resourcePanel.setOutlineThickness(2.0f);
    target.draw(resourcePanel);

    drawCoinIcon(target, {38.0f, 40.0f});
    drawNumber(target, {52.0f, 31.0f}, player.getCoins(), sf::Color(244, 235, 221), 1.1f);

    drawKeyIcon(target, {112.0f, 40.0f});
    drawNumber(target, {128.0f, 31.0f}, player.getKeys(), sf::Color(244, 235, 221), 1.1f);

    drawBombIcon(target, {184.0f, 40.0f});
    drawNumber(target, {198.0f, 31.0f}, player.getBombs(), sf::Color(244, 235, 221), 1.1f);
}

void HUD::drawBossBar(sf::RenderTarget& target, const Room& room) const {
    if (!room.hasBoss()) {
        return;
    }

    sf::RectangleShape panel({560.0f, 30.0f});
    panel.setPosition({200.0f, 674.0f});
    panel.setFillColor(sf::Color(20, 10, 10, 210));
    panel.setOutlineColor(sf::Color(130, 100, 70));
    panel.setOutlineThickness(2.0f);
    target.draw(panel);

    sf::RectangleShape barBg({536.0f, 12.0f});
    barBg.setPosition({212.0f, 683.0f});
    barBg.setFillColor(sf::Color(60, 24, 20));
    target.draw(barBg);

    sf::RectangleShape barFill({536.0f * room.getBossHpRatio(), 12.0f});
    barFill.setPosition({212.0f, 683.0f});
    barFill.setFillColor(sf::Color(200, 55, 42));
    target.draw(barFill);

    if (!m_hasFont) {
        return;
    }

    sf::Text label;
    label.setFont(m_font);
    label.setCharacterSize(18);
    label.setFillColor(sf::Color(246, 232, 210));
    label.setPosition({214.0f, 656.0f});
    label.setString("BOSS");
    target.draw(label);
}

void HUD::drawItemPickup(sf::RenderTarget& target, const Item& item, float timeRemaining) const {
    if (!m_hasFont) {
        return;
    }

    const float alpha = std::clamp(timeRemaining, 0.0f, 1.0f);

    sf::RectangleShape panel({360.0f, 72.0f});
    panel.setPosition({300.0f, 34.0f});
    panel.setFillColor(sf::Color(18, 12, 10, static_cast<std::uint8_t>(220.0f * alpha)));
    panel.setOutlineColor(sf::Color(156, 126, 74, static_cast<std::uint8_t>(255.0f * alpha)));
    panel.setOutlineThickness(2.0f);
    target.draw(panel);

    sf::CircleShape icon(11.0f);
    icon.setOrigin({11.0f, 11.0f});
    icon.setPosition({332.0f, 70.0f});
    switch (item.effect) {
    case ItemEffect::TearRate:
        icon.setFillColor(item.amount < 0.0f ? sf::Color(178, 228, 255) : sf::Color(118, 148, 188));
        break;
    case ItemEffect::Damage:
        icon.setFillColor(item.amount > 0.0f ? sf::Color(224, 72, 78) : sf::Color(110, 52, 58));
        break;
    case ItemEffect::Speed:
        icon.setFillColor(item.amount > 0.0f ? sf::Color(118, 214, 128) : sf::Color(70, 118, 74));
        break;
    }
    target.draw(icon);

    sf::Text title;
    title.setFont(m_font);
    title.setCharacterSize(24);
    title.setFillColor(sf::Color(246, 232, 210, static_cast<std::uint8_t>(255.0f * alpha)));
    title.setPosition({356.0f, 42.0f});
    title.setString(item.name);
    target.draw(title);

    sf::Text subtitle;
    subtitle.setFont(m_font);
    subtitle.setCharacterSize(18);
    subtitle.setFillColor(sf::Color(220, 206, 184, static_cast<std::uint8_t>(255.0f * alpha)));
    subtitle.setPosition({356.0f, 69.0f});
    subtitle.setString(item.description + "  (" + itemDeltaText(item) + ")");
    target.draw(subtitle);
}

void HUD::drawGameOver(sf::RenderTarget& target) const {
    if (!m_hasFont) {
        return;
    }

    sf::Text title;
    title.setFont(m_font);
    title.setCharacterSize(54);
    title.setFillColor(sf::Color(230, 218, 206));
    title.setPosition({318.0f, 248.0f});
    title.setString("GAME OVER");
    target.draw(title);

    sf::Text hint;
    hint.setFont(m_font);
    hint.setCharacterSize(22);
    hint.setFillColor(sf::Color(214, 188, 168));
    hint.setPosition({338.0f, 322.0f});
    hint.setString("Press R to restart");
    target.draw(hint);
}
