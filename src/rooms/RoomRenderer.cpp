#include "rooms/RoomRenderer.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

#include <SFML/Graphics.hpp>

#include "Room.h"

namespace {
#if SFML_VERSION_MAJOR < 3
int intRectWidth(const sf::IntRect& rect) { return rect.width; }
int intRectHeight(const sf::IntRect& rect) { return rect.height; }
void setRotationDegrees(sf::Transformable& transformable, float degrees) { transformable.setRotation(degrees); }
#else
int intRectWidth(const sf::IntRect& rect) { return rect.size.x; }
int intRectHeight(const sf::IntRect& rect) { return rect.size.y; }
void setRotationDegrees(sf::Transformable& transformable, float degrees) { transformable.setRotation(sf::degrees(degrees)); }
#endif

sf::Vector2f tileCenter(int col, int row) {
    return {
        Room::kGridLeft + static_cast<float>(col) * Room::kTileSize + Room::kTileSize * 0.5f,
        Room::kGridTop + static_cast<float>(row) * Room::kTileSize + Room::kTileSize * 0.5f
    };
}

void drawDoorImpl(float doorOpenProgress,
                  bool doorTextureLoaded,
                  const sf::Texture& doorTexture,
                  RoomType roomType,
                  sf::RenderTarget& target,
                  Direction direction) {
    const float t = std::clamp(doorOpenProgress, 0.0f, 1.0f);
    if (doorTextureLoaded) {
        constexpr sf::IntRect kFrameTopRect({8, 9}, {49, 33});
        constexpr sf::IntRect kOpenFillTopRect({84, 16}, {25, 23});
        constexpr sf::IntRect kClosedLeftRect({19, 64}, {14, 23});
        constexpr sf::IntRect kClosedRightRect({96, 64}, {14, 23});
        auto withAlpha = [](sf::Sprite& sprite, float alpha) {
            sprite.setColor(sf::Color(255, 255, 255, static_cast<std::uint8_t>(std::round(std::clamp(alpha, 0.0f, 1.0f) * 255.0f))));
        };

        switch (direction) {
        case Direction::Up:
        case Direction::Down: {
            const bool bottom = direction == Direction::Down;
            const float frameWidth = 84.0f;
            const float h = 45.0f;
            const float fillWidth = 32.0f;
            const float fillHeight = 26.0f;
            const float wallDepth = bottom ? 52.0f : 6.0f;
            const float shadowOffsetTop = 0.61f;
            const float shadowOffsetBottom = 0.60f;

            const sf::Vector2f doorPosition = bottom
                ? sf::Vector2f(tileCenter(7, 8).x - frameWidth * 0.5f, Room::kGridTop + Room::kGridRows * Room::kTileSize + wallDepth)
                : sf::Vector2f(tileCenter(7, 0).x - frameWidth * 0.5f, Room::kGridTop - h - wallDepth);

            sf::Sprite frameSprite(doorTexture);
            frameSprite.setTextureRect(kFrameTopRect);
            frameSprite.setPosition(doorPosition);
            frameSprite.setScale({frameWidth / static_cast<float>(intRectWidth(kFrameTopRect)),
                                  (h / static_cast<float>(intRectHeight(kFrameTopRect))) * (bottom ? -1.0f : 1.0f)});
            withAlpha(frameSprite, 1.0f);

            sf::Sprite fillSprite(doorTexture);
            fillSprite.setTextureRect(kOpenFillTopRect);
            fillSprite.setOrigin({static_cast<float>(intRectWidth(kOpenFillTopRect)) * 0.5f, static_cast<float>(intRectHeight(kOpenFillTopRect)) * 0.5f});
            fillSprite.setPosition({doorPosition.x + frameWidth * 0.5f, doorPosition.y + (bottom ? -h * shadowOffsetBottom : h * shadowOffsetTop)});
            fillSprite.setScale({(frameWidth * 0.7f) / static_cast<float>(intRectWidth(kOpenFillTopRect)),
                                 (h * 0.8f - 5.0f) / static_cast<float>(intRectHeight(kOpenFillTopRect)) * (bottom ? -1.0f : 1.0f)});
            withAlpha(fillSprite, t);
            target.draw(fillSprite);

            sf::Sprite closedLeftSprite(doorTexture);
            sf::Sprite closedRightSprite(doorTexture);
            const float closedScaleX = (fillWidth * 0.5f) / static_cast<float>(intRectWidth(kClosedLeftRect));
            const float closedScaleY = (fillHeight * 1.1f - 5.0f) / static_cast<float>(intRectHeight(kClosedLeftRect));

            closedLeftSprite.setTextureRect(kClosedLeftRect);
            closedLeftSprite.setOrigin({static_cast<float>(intRectWidth(kClosedLeftRect)), static_cast<float>(intRectHeight(kClosedLeftRect)) * 0.5f});
            closedLeftSprite.setPosition({doorPosition.x + frameWidth * 0.5f, doorPosition.y + (bottom ? -h * shadowOffsetBottom : h * shadowOffsetTop)});
            closedLeftSprite.setScale({closedScaleX * 1.5f, closedScaleY * 1.5f * (bottom ? -1.0f : 1.0f)});
            withAlpha(closedLeftSprite, 1.0f - t);
            target.draw(closedLeftSprite);

            closedRightSprite.setTextureRect(kClosedRightRect);
            closedRightSprite.setOrigin({0.0f, static_cast<float>(intRectHeight(kClosedRightRect)) * 0.5f});
            closedRightSprite.setPosition({doorPosition.x + frameWidth * 0.5f, doorPosition.y + (bottom ? -h * shadowOffsetBottom : h * shadowOffsetTop)});
            closedRightSprite.setScale({closedScaleX * 1.5f, closedScaleY * 1.5f * (bottom ? -1.0f : 1.0f)});
            withAlpha(closedRightSprite, 1.0f - t);
            target.draw(closedRightSprite);

            target.draw(frameSprite);
            return;
        }
        case Direction::Left:
        case Direction::Right: {
            const bool right = direction == Direction::Right;
            const float frameWidth = 54.0f;
            const float frameHeight = 84.0f;
            const float fillWidth = 28.0f;
            const float fillHeight = 36.0f;
            const float wallDepth = 34.0f;
            const float centerX = right
                ? (Room::kGridLeft + Room::kGridCols * Room::kTileSize + wallDepth)
                : Room::kGridLeft - wallDepth;
            const float centerY = tileCenter(right ? 14 : 0, 4).y;
            const float rotation = right ? 90.0f : -90.0f;

            sf::Sprite frameSprite(doorTexture);
            frameSprite.setTextureRect(kFrameTopRect);
            frameSprite.setOrigin({static_cast<float>(intRectWidth(kFrameTopRect)) * 0.5f, static_cast<float>(intRectHeight(kFrameTopRect)) * 0.5f});
            frameSprite.setPosition({centerX, centerY});
            setRotationDegrees(frameSprite, rotation);
            frameSprite.setScale({frameHeight / static_cast<float>(intRectWidth(kFrameTopRect)),
                                  frameWidth / static_cast<float>(intRectHeight(kFrameTopRect))});
            withAlpha(frameSprite, 1.0f);

            sf::Sprite fillSprite(doorTexture);
            fillSprite.setTextureRect(kOpenFillTopRect);
            fillSprite.setOrigin({static_cast<float>(intRectWidth(kOpenFillTopRect)) * 0.5f, static_cast<float>(intRectHeight(kOpenFillTopRect)) * 0.5f});
            fillSprite.setPosition({centerX + (right ? -6.0f : 6.0f), centerY + 2.0f});
            setRotationDegrees(fillSprite, rotation);
            fillSprite.setScale({(frameHeight * 0.7f) / static_cast<float>(intRectWidth(kOpenFillTopRect)),
                                 (frameWidth * 0.75f) / static_cast<float>(intRectHeight(kOpenFillTopRect))});
            withAlpha(fillSprite, t);
            target.draw(fillSprite);

            sf::Sprite closedLeftSprite(doorTexture);
            sf::Sprite closedRightSprite(doorTexture);
            const float sideClosedScaleX = (fillHeight * 0.6f) / static_cast<float>(intRectWidth(kClosedLeftRect));
            const float sideClosedScaleY = (fillWidth * 1.1f + 3.0f) / static_cast<float>(intRectHeight(kClosedLeftRect));

            closedLeftSprite.setTextureRect(kClosedLeftRect);
            closedLeftSprite.setOrigin({static_cast<float>(intRectWidth(kClosedLeftRect)), static_cast<float>(intRectHeight(kClosedLeftRect)) * 0.5f});
            closedLeftSprite.setPosition({centerX + (right ? -5.0f : 5.0f), centerY + 1.0f});
            setRotationDegrees(closedLeftSprite, rotation);
            closedLeftSprite.setScale({sideClosedScaleX, sideClosedScaleY});
            withAlpha(closedLeftSprite, 1.0f - t);
            target.draw(closedLeftSprite);

            closedRightSprite.setTextureRect(kClosedRightRect);
            closedRightSprite.setOrigin({0.0f, static_cast<float>(intRectHeight(kClosedRightRect)) * 0.5f});
            closedRightSprite.setPosition({centerX + (right ? -5.0f : 5.0f), centerY + 1.0f});
            setRotationDegrees(closedRightSprite, rotation);
            closedRightSprite.setScale({sideClosedScaleX, sideClosedScaleY});
            withAlpha(closedRightSprite, 1.0f - t);
            target.draw(closedRightSprite);

            target.draw(frameSprite);
            return;
        }
        }
    }

    sf::RectangleShape door;
    const auto blend = [t](std::uint8_t closed, std::uint8_t open) -> std::uint8_t {
        return static_cast<std::uint8_t>(std::round(static_cast<float>(closed) + (static_cast<float>(open) - static_cast<float>(closed)) * t));
    };

    const bool treasureTint = roomType == RoomType::Treasure;
    const sf::Color closedColor = treasureTint ? sf::Color(126, 92, 36) : sf::Color(90, 60, 40);
    const sf::Color openColor = treasureTint ? sf::Color(222, 184, 72) : sf::Color(190, 160, 95);
    door.setFillColor(sf::Color(blend(closedColor.r, openColor.r),
                                blend(closedColor.g, openColor.g),
                                blend(closedColor.b, openColor.b)));

    switch (direction) {
    case Direction::Up:
        door.setSize({Room::kTileSize, Room::kDoorThickness * (1.0f - 0.6f * t)});
        door.setPosition({tileCenter(7, 0).x - Room::kTileSize * 0.5f, Room::kGridTop - door.getSize().y});
        break;
    case Direction::Down:
        door.setSize({Room::kTileSize, Room::kDoorThickness * (1.0f - 0.6f * t)});
        door.setPosition({tileCenter(7, 8).x - Room::kTileSize * 0.5f, Room::kGridTop + Room::kGridRows * Room::kTileSize});
        break;
    case Direction::Left:
        door.setSize({Room::kDoorThickness * (1.0f - 0.6f * t), Room::kTileSize});
        door.setPosition({Room::kGridLeft - door.getSize().x, tileCenter(0, 4).y - Room::kTileSize * 0.5f});
        break;
    case Direction::Right:
        door.setSize({Room::kDoorThickness * (1.0f - 0.6f * t), Room::kTileSize});
        door.setPosition({Room::kGridLeft + Room::kGridCols * Room::kTileSize, tileCenter(14, 4).y - Room::kTileSize * 0.5f});
        break;
    }

    target.draw(door);
}
}

void RoomRenderer::drawDoor(const Room& room, sf::RenderTarget& target, Direction direction) {
    drawDoorImpl(room.m_doorOpenProgress, Room::s_doorTextureLoaded, Room::s_doorTexture, room.m_roomType, target, direction);
}

void RoomRenderer::draw(const Room& room, sf::RenderTarget& target) {
    target.draw(room.m_floor);

    if (room.m_floorSprite.has_value()) {
        target.draw(*room.m_floorSprite);
    } else if (room.m_backdropSprite.has_value()) {
        target.draw(*room.m_backdropSprite);
    }

    if (room.m_cornerSprites[0].has_value()) {
        for (int i = 0; i < 4; ++i) {
            target.draw(*room.m_cornerSprites[i]);
        }

        for (int i = 0; i < 4; ++i) {
            if (room.m_wallSprites[i].has_value()) {
                target.draw(*room.m_wallSprites[i]);
            }
        }
    } else if (!room.m_floorSprite.has_value() && !room.m_backdropSprite.has_value()) {
        target.draw(room.m_innerBounds);
    }

    for (const auto& rock : room.m_rocks) {
        target.draw(rock);
    }
    for (const auto& prop : room.m_props) {
        target.draw(prop.shape);
    }
    if (room.m_reward.has_value()) {
        target.draw(room.m_reward->pedestal);
        target.draw(room.m_reward->icon);
    }

    for (int i = 0; i < 4; ++i) {
        if (room.m_doors[i]) {
            RoomRenderer::drawDoor(room, target, static_cast<Direction>(i));
        }
    }

    for (const auto& pickup : room.m_pickups) {
        target.draw(pickup.shape);
    }
    for (const auto& monster : room.m_monsters) {
        monster->draw(target);
    }
}
