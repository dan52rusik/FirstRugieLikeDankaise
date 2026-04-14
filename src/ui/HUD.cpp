#include "HUD.h"

#include "../Player.h"
#include "../Room.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>

namespace {
#if SFML_VERSION_MAJOR < 3
bool loadFont(sf::Font& font, const char* path) {
    if (font.loadFromFile(path)) {
        // SFML 2 требует вызова getTexture для генерации глифов. 
        // Мы используем const_cast, чтобы включить сглаживание у текстуры шрифта.
        for (unsigned int size : {12, 14, 16, 18, 20, 21, 22, 24, 32, 48, 54}) {
            const_cast<sf::Texture&>(font.getTexture(size)).setSmooth(true);
        }
        return true;
    }
    return false;
}

void setRotationDegrees(sf::Transformable& transformable, float degrees) {
    transformable.setRotation(degrees);
}
#else
bool loadFont(sf::Font& font, const char* path) {
    if (font.openFromFile(path)) {
        // Включаем сглаживание для основных размеров шрифта
        const_cast<sf::Texture&>(font.getTexture(24)).setSmooth(true);
        const_cast<sf::Texture&>(font.getTexture(16)).setSmooth(true);
        return true;
    }
    return false;
}

void setRotationDegrees(sf::Transformable& transformable, float degrees) {
    transformable.setRotation(sf::degrees(degrees));
}
#endif

// Вспомогательная функция перевода координат из логических 960x720 в пиксели окна
inline sf::Vector2f toScreen(sf::Vector2f logicalPos, sf::Vector2f origin, float scale) {
    return {origin.x + logicalPos.x * scale, origin.y + logicalPos.y * scale};
}

sf::Text makeText(const sf::Font& font,
                  unsigned int logicalSize,
                  sf::Color color,
                  sf::Vector2f screenPos,
                  float scale,
                  const sf::String& string = {}) {
    // В SFML 3 конструктор изменился: строки идут перед шрифтом
    sf::Text text("", font);
    text.setString(string);
    text.setCharacterSize(static_cast<unsigned int>(static_cast<float>(logicalSize) * scale));
    text.setFillColor(color);
    text.setPosition(screenPos);
    return text;
}

std::string formatFloat(float value, int decimals) {
    const float scale = std::pow(10.0f, static_cast<float>(decimals));
    const int scaled = static_cast<int>(std::round(std::abs(value) * scale));
    const int whole = scaled / static_cast<int>(scale);
    const int frac = scaled % static_cast<int>(scale);

    std::string text = std::to_string(whole);
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

std::string formatSignedFloat(float value, int decimals) {
    const bool positive = value >= 0.0f;
    std::string text = positive ? "+" : "-";
    text += formatFloat(value, decimals);
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

sf::Color itemAccentColor(const ItemEffect effect, float amount, std::uint8_t alpha = 255) {
    switch (effect) {
    case ItemEffect::TearRate:
        return amount < 0.0f ? sf::Color(118, 214, 128, alpha) : sf::Color(214, 96, 96, alpha);
    case ItemEffect::Damage:
    case ItemEffect::Speed:
        return amount > 0.0f ? sf::Color(118, 214, 128, alpha) : sf::Color(214, 96, 96, alpha);
    }
    return sf::Color(220, 220, 220, alpha);
}

std::string statValueText(const Player& player, ItemEffect effect) {
    switch (effect) {
    case ItemEffect::Damage:
        return formatFloat(player.getTearDamage(), 1);
    case ItemEffect::TearRate:
        return formatFloat(player.getTearDelay(), 2) + "s";
    case ItemEffect::Speed:
        return formatFloat(player.getMoveSpeed(), 0);
    }
    return {};
}

std::string statDeltaText(const Player& player, const Item& item) {
    const float current = item.effect == ItemEffect::Damage
        ? player.getTearDamage()
        : item.effect == ItemEffect::TearRate ? player.getTearDelay() : player.getMoveSpeed();

    const float previous = current - item.amount;
    std::string text = formatSignedFloat(item.amount, item.effect == ItemEffect::Damage ? 1 : item.effect == ItemEffect::TearRate ? 2 : 0);
    if (item.effect == ItemEffect::TearRate) {
        text += "s";
    }
    if (std::abs(previous) > 0.001f) {
        const int percent = static_cast<int>(std::round((item.amount / previous) * 100.0f));
        if (percent != 0) {
            text += "  (" + formatSignedFloat(static_cast<float>(percent), 0) + "%)";
        }
    }
    return text;
}
}

HUD::HUD() : m_hasFont(false) {
#ifdef __EMSCRIPTEN__
    static const char* kFontPaths[] = {
        "/assets/fonts/isaac.ttf",
        "assets/fonts/isaac.ttf"
    };
#else
    static const char* kFontPaths[] = {
        "assets/fonts/isaac.ttf",
        "C:/Windows/Fonts/trebuc.ttf",
        "C:/Windows/Fonts/arial.ttf",
        "C:/Windows/Fonts/segoeui.ttf"
    };
#endif

    for (const char* path : kFontPaths) {
        if (loadFont(m_font, path)) {
            m_hasFont = true;
            break;
        }
    }
}

void HUD::drawDigit(sf::RenderTarget& target, sf::Vector2f position, int digit, sf::Color color, float scale, sf::Vector2f origin, float uiScale) const {
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

    // Базовые логические размеры сегментов
    const float baseW = 10.0f;
    const float baseH = 18.0f;
    const float baseT = 2.0f;

    auto drawSegment = [&](sf::Vector2f logicalOffset, sf::Vector2f logicalSize) {
        // Итоговый размер: (базовый * масштаб_шрифта) * масштаб_экрана
        const sf::Vector2f physicalSize(logicalSize.x * scale * uiScale, logicalSize.y * scale * uiScale);
        sf::RectangleShape rect(physicalSize);
        
        // Итоговая позиция: экранная_база + (логическая_база + смещение_сегмента * масштаб_шрифта) * масштаб_экрана
        const sf::Vector2f logicalPos = position + logicalOffset * scale;
        rect.setPosition(toScreen(logicalPos, origin, uiScale));
        
        rect.setFillColor(color);
        target.draw(rect);
    };

    if (kSegments[digit][0]) drawSegment({0.0f, 0.0f}, {baseW, baseT});
    if (kSegments[digit][1]) drawSegment({0.0f, 0.0f}, {baseT, baseH * 0.5f});
    if (kSegments[digit][2]) drawSegment({baseW - baseT, 0.0f}, {baseT, baseH * 0.5f});
    if (kSegments[digit][3]) drawSegment({0.0f, baseH * 0.5f - baseT * 0.5f}, {baseW, baseT});
    if (kSegments[digit][4]) drawSegment({0.0f, baseH * 0.5f}, {baseT, baseH * 0.5f});
    if (kSegments[digit][5]) drawSegment({baseW - baseT, baseH * 0.5f}, {baseT, baseH * 0.5f});
    if (kSegments[digit][6]) drawSegment({0.0f, baseH - baseT}, {baseW, baseT});
}

void HUD::drawNumber(sf::RenderTarget& target, sf::Vector2f position, int value, sf::Color color, float scale, sf::Vector2f origin, float uiScale) const {
    const std::string digits = std::to_string(std::max(0, value));
    for (std::size_t i = 0; i < digits.size(); ++i) {
        drawDigit(target, {position.x + static_cast<float>(i) * 13.0f * scale, position.y}, digits[i] - '0', color, scale, origin, uiScale);
    }
}

void HUD::drawCoinIcon(sf::RenderTarget& target, sf::Vector2f center, sf::Vector2f origin, float scale) const {
    sf::CircleShape coin(6.0f * scale);
    coin.setOrigin({6.0f * scale, 6.0f * scale});
    coin.setPosition(toScreen(center, origin, scale));
    coin.setFillColor(sf::Color(236, 204, 82));
    target.draw(coin);

    sf::RectangleShape shine({4.0f * scale, 2.0f * scale});
    shine.setPosition(toScreen({center.x - 1.0f, center.y - 4.0f}, origin, scale));
    shine.setFillColor(sf::Color(255, 236, 154));
    target.draw(shine);
}

void HUD::drawKeyIcon(sf::RenderTarget& target, sf::Vector2f center, sf::Vector2f origin, float scale) const {
    sf::CircleShape ring(4.0f * scale);
    ring.setOrigin({4.0f * scale, 4.0f * scale});
    ring.setPosition(toScreen({center.x - 4.0f, center.y}, origin, scale));
    ring.setFillColor(sf::Color::Transparent);
    ring.setOutlineThickness(2.0f * scale);
    ring.setOutlineColor(sf::Color(210, 210, 210));
    target.draw(ring);

    sf::RectangleShape shaft({11.0f * scale, 2.0f * scale});
    shaft.setPosition(toScreen({center.x - 1.0f, center.y - 1.0f}, origin, scale));
    shaft.setFillColor(sf::Color(210, 210, 210));
    target.draw(shaft);

    sf::RectangleShape toothA({2.0f * scale, 4.0f * scale});
    toothA.setPosition(toScreen({center.x + 6.0f, center.y - 1.0f}, origin, scale));
    toothA.setFillColor(sf::Color(210, 210, 210));
    target.draw(toothA);

    sf::RectangleShape toothB({2.0f * scale, 3.0f * scale});
    toothB.setPosition(toScreen({center.x + 10.0f, center.y - 1.0f}, origin, scale));
    toothB.setFillColor(sf::Color(210, 210, 210));
    target.draw(toothB);
}

void HUD::drawBombIcon(sf::RenderTarget& target, sf::Vector2f center, sf::Vector2f origin, float scale) const {
    sf::CircleShape bomb(6.0f * scale);
    bomb.setOrigin({6.0f * scale, 6.0f * scale});
    bomb.setPosition(toScreen(center, origin, scale));
    bomb.setFillColor(sf::Color(54, 54, 58));
    target.draw(bomb);

    sf::RectangleShape fuse({5.0f * scale, 2.0f * scale});
    fuse.setPosition(toScreen({center.x + 2.0f, center.y - 7.0f}, origin, scale));
    setRotationDegrees(fuse, -35.0f);
    fuse.setFillColor(sf::Color(184, 132, 72));
    target.draw(fuse);

    sf::CircleShape spark(2.0f * scale);
    spark.setOrigin({2.0f * scale, 2.0f * scale});
    spark.setPosition(toScreen({center.x + 7.0f, center.y - 9.0f}, origin, scale));
    spark.setFillColor(sf::Color(250, 210, 94));
    target.draw(spark);
}

void HUD::drawHeart(sf::RenderTarget& target, sf::Vector2f center, sf::Color color, float scale, sf::Vector2f origin, float uiScale) const {
    sf::CircleShape leftLobe(7.0f * scale * uiScale);
    leftLobe.setOrigin({7.0f * scale * uiScale, 7.0f * scale * uiScale});
    leftLobe.setPosition(toScreen({center.x - 5.0f * scale, center.y - 5.0f * scale}, origin, uiScale));
    leftLobe.setFillColor(color);

    sf::CircleShape rightLobe(7.0f * scale * uiScale);
    rightLobe.setOrigin({7.0f * scale * uiScale, 7.0f * scale * uiScale});
    rightLobe.setPosition(toScreen({center.x + 5.0f * scale, center.y - 5.0f * scale}, origin, uiScale));
    rightLobe.setFillColor(color);

    sf::ConvexShape tip(3);
    tip.setPoint(0, toScreen({center.x - 13.0f * scale, center.y - 1.0f * scale}, origin, uiScale));
    tip.setPoint(1, toScreen({center.x + 13.0f * scale, center.y - 1.0f * scale}, origin, uiScale));
    tip.setPoint(2, toScreen({center.x, center.y + 16.0f * scale}, origin, uiScale));
    tip.setFillColor(color);

    target.draw(leftLobe);
    target.draw(rightLobe);
    target.draw(tip);
}

void HUD::draw(sf::RenderTarget& target, const Player& player, sf::Vector2f origin, float scale) const {
    const int hearts = (player.getMaxHp() + 1) / 2;
    const int currentHp = player.getHp();

    sf::RectangleShape heartPanel({122.0f * scale, 48.0f * scale});
    heartPanel.setPosition(toScreen({14.0f, 658.0f}, origin, scale));
    heartPanel.setFillColor(sf::Color(18, 12, 10, 180));
    heartPanel.setOutlineColor(sf::Color(88, 64, 52, 220));
    heartPanel.setOutlineThickness(2.0f * scale);
    target.draw(heartPanel);

    for (int i = 0; i < hearts; ++i) {
        const float x = 34.0f + static_cast<float>(i) * 30.0f;
        const float y = 682.0f;
        const sf::Vector2f center(x, y);

        drawHeart(target, center, sf::Color(55, 22, 24), 0.95f, origin, scale);

        const int hpForHeart = currentHp - i * 2;
        if (hpForHeart >= 2) {
            drawHeart(target, center, sf::Color(215, 42, 60), 0.85f, origin, scale);
        } else if (hpForHeart == 1) {
            drawHeart(target, center, sf::Color(238, 144, 152), 0.85f, origin, scale);
        }
    }

    sf::RectangleShape resourcePanel({230.0f * scale, 44.0f * scale});
    resourcePanel.setPosition(toScreen({18.0f, 18.0f}, origin, scale));
    resourcePanel.setFillColor(sf::Color(18, 12, 10, 176));
    resourcePanel.setOutlineColor(sf::Color(88, 64, 52, 220));
    resourcePanel.setOutlineThickness(2.0f * scale);
    target.draw(resourcePanel);

    drawCoinIcon(target, {38.0f, 40.0f}, origin, scale);
    drawNumber(target, {52.0f, 31.0f}, player.getCoins(), sf::Color(244, 235, 221), 1.1f, origin, scale);

    drawKeyIcon(target, {112.0f, 40.0f}, origin, scale);
    drawNumber(target, {128.0f, 31.0f}, player.getKeys(), sf::Color(244, 235, 221), 1.1f, origin, scale);

    drawBombIcon(target, {184.0f, 40.0f}, origin, scale);
    drawNumber(target, {198.0f, 31.0f}, player.getBombs(), sf::Color(244, 235, 221), 1.1f, origin, scale);
}

void HUD::drawStatsPanel(sf::RenderTarget& target,
                         const Player& player,
                         const std::optional<Item>& recentItem,
                         float recentItemAlpha,
                         bool expanded,
                         sf::Vector2f origin, float scale) const {
    if (!m_hasFont) {
        return;
    }

    const sf::Vector2f panelSize = expanded ? sf::Vector2f(246.0f * scale, 184.0f * scale) : sf::Vector2f(212.0f * scale, 118.0f * scale);
    const sf::Vector2f panelLogicalPos(18.0f, 72.0f);
    const sf::Vector2f panelPosition = toScreen(panelLogicalPos, origin, scale);
    
    const unsigned int titleSize = expanded ? 18u : 14u;
    const unsigned int valueSize = expanded ? 21u : 16u;
    const unsigned int deltaSize = expanded ? 16u : 12u;
    const float rowStep = expanded ? 34.0f : 24.0f;
    const float rowStart = expanded ? 40.0f : 24.0f;

    sf::RectangleShape panel(panelSize);
    panel.setPosition(panelPosition);
    panel.setFillColor(sf::Color(18, 12, 10, expanded ? 210 : 176));
    panel.setOutlineColor(sf::Color(88, 64, 52, 220));
    panel.setOutlineThickness(2.0f * scale);
    target.draw(panel);

    if (expanded) {
        sf::Text title = makeText(m_font, titleSize, sf::Color(232, 220, 202),
                                  toScreen({panelLogicalPos.x + 12.0f, panelLogicalPos.y + 8.0f}, origin, scale), scale, "STATS");
        target.draw(title);
    }

    const Item* highlight = recentItemAlpha > 0.0f && recentItem.has_value() ? &*recentItem : nullptr;
    const std::uint8_t deltaAlpha = static_cast<std::uint8_t>(255.0f * std::clamp(recentItemAlpha, 0.0f, 1.0f));

    const auto drawLine = [&](const std::string& label,
                              const std::string& value,
                              ItemEffect effect,
                              const std::string& fallbackDelta,
                              float y) {
        sf::Text labelText = makeText(m_font, deltaSize, sf::Color(176, 158, 142),
                                      toScreen({panelLogicalPos.x + 12.0f, y}, origin, scale), scale, label);
        target.draw(labelText);

        sf::Text valueText = makeText(m_font, valueSize, sf::Color(244, 235, 221),
                                      toScreen({panelLogicalPos.x + 82.0f, y - 4.0f}, origin, scale), scale, value);
        target.draw(valueText);

        if (highlight != nullptr && highlight->effect == effect) {
            sf::Text deltaText = makeText(m_font, deltaSize, itemAccentColor(effect, highlight->amount, deltaAlpha),
                                          toScreen({panelLogicalPos.x + 132.0f, y}, origin, scale), scale, statDeltaText(player, *highlight));
            target.draw(deltaText);
        } else if (expanded && !fallbackDelta.empty()) {
            sf::Text deltaText = makeText(m_font, deltaSize, sf::Color(110, 96, 86),
                                          toScreen({panelLogicalPos.x + 132.0f, y}, origin, scale), scale, fallbackDelta);
            target.draw(deltaText);
        }
    };

    drawLine("DMG", statValueText(player, ItemEffect::Damage), ItemEffect::Damage, "", panelLogicalPos.y + rowStart);
    drawLine("TEARS", statValueText(player, ItemEffect::TearRate), ItemEffect::TearRate, "delay", panelLogicalPos.y + rowStart + rowStep);
    drawLine("SPEED", statValueText(player, ItemEffect::Speed), ItemEffect::Speed, "move", panelLogicalPos.y + rowStart + rowStep * 2.0f);

    sf::Text luckLabel = makeText(m_font, deltaSize, sf::Color(176, 158, 142),
                                  toScreen({panelLogicalPos.x + 12.0f, panelLogicalPos.y + rowStart + rowStep * 3.0f}, origin, scale), scale, "LUCK");
    target.draw(luckLabel);

    sf::Text luckValue = makeText(m_font, valueSize, sf::Color(244, 235, 221),
                                  toScreen({panelLogicalPos.x + 82.0f, panelLogicalPos.y + rowStart + rowStep * 3.0f - 4.0f}, origin, scale), scale,
                                  formatFloat(player.getLuck(), 1));
    target.draw(luckValue);

    // Метка версии
    sf::Text version = makeText(m_font, 12, sf::Color(100, 100, 100), toScreen({10.0f, 700.0f}, origin, scale), scale, "v0.3-SHARP");
    target.draw(version);
}

void HUD::drawBossBar(sf::RenderTarget& target, const Room& room, sf::Vector2f origin, float scale) const {
    if (!room.hasBoss()) {
        return;
    }

    sf::RectangleShape panel({560.0f * scale, 30.0f * scale});
    panel.setPosition(toScreen({200.0f, 674.0f}, origin, scale));
    panel.setFillColor(sf::Color(20, 10, 10, 210));
    panel.setOutlineColor(sf::Color(130, 100, 70));
    panel.setOutlineThickness(2.0f * scale);
    target.draw(panel);

    sf::RectangleShape barBg({536.0f * scale, 12.0f * scale});
    barBg.setPosition(toScreen({212.0f, 683.0f}, origin, scale));
    barBg.setFillColor(sf::Color(60, 24, 20));
    target.draw(barBg);

    sf::RectangleShape barFill({536.0f * scale * room.getBossHpRatio(), 12.0f * scale});
    barFill.setPosition(toScreen({212.0f, 683.0f}, origin, scale));
    barFill.setFillColor(sf::Color(200, 55, 42));
    target.draw(barFill);

    if (!m_hasFont) {
        return;
    }

    sf::Text label = makeText(m_font, 18, sf::Color(246, 232, 210), toScreen({214.0f, 656.0f}, origin, scale), scale, "BOSS");
    target.draw(label);
}

void HUD::drawItemPickup(sf::RenderTarget& target, const Item& item, float timeRemaining, sf::Vector2f origin, float scale) const {
    if (!m_hasFont) {
        return;
    }

    const float alpha = std::clamp(timeRemaining, 0.0f, 1.0f);

    sf::RectangleShape panel({360.0f * scale, 72.0f * scale});
    panel.setPosition(toScreen({300.0f, 34.0f}, origin, scale));
    panel.setFillColor(sf::Color(18, 12, 10, static_cast<std::uint8_t>(220.0f * alpha)));
    panel.setOutlineColor(sf::Color(156, 126, 74, static_cast<std::uint8_t>(255.0f * alpha)));
    panel.setOutlineThickness(2.0f * scale);
    target.draw(panel);

    sf::CircleShape icon(11.0f * scale);
    icon.setOrigin({11.0f * scale, 11.0f * scale});
    icon.setPosition(toScreen({332.0f, 70.0f}, origin, scale));
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

    sf::Text title = makeText(
        m_font, 24,
        sf::Color(246, 232, 210, static_cast<std::uint8_t>(255.0f * alpha)),
        toScreen({356.0f, 42.0f}, origin, scale), scale, item.name);
    target.draw(title);

    sf::Text subtitle = makeText(
        m_font, 18,
        sf::Color(220, 206, 184, static_cast<std::uint8_t>(255.0f * alpha)),
        toScreen({356.0f, 69.0f}, origin, scale), scale, item.description + "  (" + itemDeltaText(item) + ")");
    target.draw(subtitle);
}

void HUD::drawGameOver(sf::RenderTarget& target, sf::Vector2f origin, float scale) const {
    if (!m_hasFont) {
        return;
    }

    sf::Text title = makeText(m_font, 54, sf::Color(230, 218, 206), toScreen({318.0f, 248.0f}, origin, scale), scale, "GAME OVER");
    target.draw(title);

    sf::Text hint = makeText(m_font, 22, sf::Color(214, 188, 168), toScreen({338.0f, 322.0f}, origin, scale), scale, "Press R to restart");
    target.draw(hint);
}
