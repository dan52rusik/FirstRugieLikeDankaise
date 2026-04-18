#include "rooms/RoomVisualBuilder.h"

#include <SFML/Graphics.hpp>

#include "Room.h"

void RoomVisualBuilder::initialize(Room& room) {
    room.m_floor.setSize({Room::kRoomWidth, Room::kRoomHeight});
    room.m_floor.setPosition({Room::kRoomLeft, Room::kRoomTop});
    room.m_floor.setFillColor(sf::Color(30, 80, 150));

    room.m_innerBounds.setSize(room.m_floor.getSize());
    room.m_innerBounds.setPosition(room.m_floor.getPosition());
    room.m_innerBounds.setFillColor(sf::Color::Transparent);
    room.m_innerBounds.setOutlineThickness(20.0f);
    room.m_innerBounds.setOutlineColor(sf::Color::White);

    if (Room::s_backdropTextureLoaded) {
        const sf::Vector2u backdropSize = Room::s_backdropTexture.getSize();
        const int frameW = static_cast<int>(backdropSize.x / 2);
        const int frameH = static_cast<int>(backdropSize.y / 3);
        const int variant = 0;
        const int texX = (variant % 2) * frameW;
        const int texY = (variant / 2) * frameH;

        room.m_backdropSprite.emplace(Room::s_backdropTexture);
        room.m_backdropSprite->setTextureRect(sf::IntRect(sf::Vector2i(texX, texY), sf::Vector2i(frameW, frameH)));
        room.m_backdropSprite->setPosition({Room::kRoomLeft, Room::kRoomTop});
        room.m_backdropSprite->setScale({
            Room::kRoomWidth / static_cast<float>(frameW),
            Room::kRoomHeight / static_cast<float>(frameH)});
    }

    if (Room::s_wallTextureLoaded) {
        for (int i = 0; i < 4; ++i) {
            room.m_cornerSprites[i].emplace(Room::s_wallTexture);
        }
    }
}

void RoomVisualBuilder::applyTheme(Room& room) {
    switch (room.m_roomType) {
    case RoomType::Treasure:
        room.m_floor.setFillColor(sf::Color(92, 72, 36));
        room.m_innerBounds.setOutlineColor(sf::Color(122, 92, 42));
        break;
    case RoomType::Boss:
        room.m_floor.setFillColor(sf::Color(82, 48, 42));
        room.m_innerBounds.setOutlineColor(sf::Color(60, 26, 22));
        break;
    default:
        room.m_floor.setFillColor(sf::Color(72, 54, 42));
        room.m_innerBounds.setOutlineColor(sf::Color(42, 28, 22));
        break;
    }
}

void RoomVisualBuilder::buildVisuals(Room& room) {
    const sf::Texture* wallTexture = Room::s_backdropTextureLoaded ? &Room::s_backdropTexture : (Room::s_wallTextureLoaded ? &Room::s_wallTexture : nullptr);
    if (wallTexture == nullptr || !room.m_cornerSprites[0].has_value()) {
        return;
    }

    const sf::Vector2u texSize = wallTexture->getSize();
    const int frameW = static_cast<int>(texSize.x / 2);
    const int frameH = static_cast<int>(texSize.y / 3);
    const int variant = 0;
    const int texX = (variant % 2) * frameW;
    const int texY = (variant / 2) * frameH;

    const bool usingIsaacBackdrop = wallTexture == &Room::s_backdropTexture;
    const int cornerSizeW = usingIsaacBackdrop ? 57 : 250;
    const int cornerSizeH = usingIsaacBackdrop ? 57 : 200;
    const float targetW = Room::kGridLeft - Room::kRoomLeft;
    const float targetH = Room::kGridTop - Room::kRoomTop;
    const float wallGapW = Room::kRoomWidth - targetW * 2.0f;
    const float wallGapH = Room::kRoomHeight - targetH * 2.0f;

    for (int i = 0; i < 4; ++i) {
        if (!room.m_wallSprites[i].has_value()) {
            room.m_wallSprites[i].emplace(*wallTexture);
        }
    }

    if (usingIsaacBackdrop) {
        room.m_floorSprite.reset();
        const int floorLeft = 57;
        const int floorTop = 57;
        const int floorTexW = 176;
        const int floorTexH = 86;

        room.m_backdropSprite.emplace(Room::s_backdropTexture);
        room.m_backdropSprite->setTextureRect(sf::IntRect(
            sf::Vector2i(texX + floorLeft, texY + floorTop),
            sf::Vector2i(floorTexW, floorTexH)));
        room.m_backdropSprite->setPosition({Room::kGridLeft, Room::kGridTop});
        room.m_backdropSprite->setScale({
            (Room::kGridCols * Room::kTileSize) / static_cast<float>(floorTexW),
            (Room::kGridRows * Room::kTileSize) / static_cast<float>(floorTexH)});

        for (int i = 0; i < 4; ++i) {
            room.m_cornerSprites[i].emplace(Room::s_backdropTexture);
            room.m_wallSprites[i].emplace(Room::s_backdropTexture);
        }

        const int wallLeft = 57;
        const int wallTop = 0;
        const int topWallTexW = 176;
        const int topWallTexH = 57;
        const int leftWallTexW = 57;
        const int leftWallTexH = 86;

        for (int i = 0; i < 4; ++i) {
            auto& sprite = *room.m_cornerSprites[i];
            const bool rightSide = (i % 2 == 1);
            const bool bottomSide = (i / 2 == 1);
            sprite.setTextureRect(sf::IntRect(sf::Vector2i(texX, texY), sf::Vector2i(cornerSizeW, cornerSizeH)));
            sprite.setOrigin({0.0f, 0.0f});
            const float posX = Room::kRoomLeft + (rightSide ? Room::kRoomWidth : 0.0f);
            const float posY = Room::kRoomTop + (bottomSide ? Room::kRoomHeight : 0.0f);
            sprite.setPosition({posX, posY});
            sprite.setScale({
                (targetW / static_cast<float>(cornerSizeW)) * (rightSide ? -1.0f : 1.0f),
                (targetH / static_cast<float>(cornerSizeH)) * (bottomSide ? -1.0f : 1.0f)});
        }

        auto& topWall = *room.m_wallSprites[0];
        topWall.setTextureRect(sf::IntRect(sf::Vector2i(texX + wallLeft, texY + wallTop), sf::Vector2i(topWallTexW, topWallTexH)));
        topWall.setOrigin({0.0f, 0.0f});
        topWall.setPosition({Room::kRoomLeft + targetW, Room::kRoomTop});
        topWall.setScale({wallGapW / static_cast<float>(topWallTexW), targetH / static_cast<float>(topWallTexH)});

        auto& bottomWall = *room.m_wallSprites[1];
        bottomWall.setTextureRect(sf::IntRect(sf::Vector2i(texX + wallLeft, texY + wallTop), sf::Vector2i(topWallTexW, topWallTexH)));
        bottomWall.setOrigin({0.0f, 0.0f});
        bottomWall.setPosition({Room::kRoomLeft + targetW, Room::kRoomTop + Room::kRoomHeight});
        bottomWall.setScale({wallGapW / static_cast<float>(topWallTexW), -(targetH / static_cast<float>(topWallTexH))});

        auto& leftWall = *room.m_wallSprites[2];
        leftWall.setTextureRect(sf::IntRect(sf::Vector2i(texX, texY + floorTop), sf::Vector2i(leftWallTexW, leftWallTexH)));
        leftWall.setOrigin({0.0f, 0.0f});
        leftWall.setPosition({Room::kRoomLeft, Room::kRoomTop + targetH});
        leftWall.setScale({targetW / static_cast<float>(leftWallTexW), wallGapH / static_cast<float>(leftWallTexH)});

        auto& rightWall = *room.m_wallSprites[3];
        rightWall.setTextureRect(sf::IntRect(sf::Vector2i(texX, texY + floorTop), sf::Vector2i(leftWallTexW, leftWallTexH)));
        rightWall.setOrigin({0.0f, 0.0f});
        rightWall.setPosition({Room::kRoomLeft + Room::kRoomWidth, Room::kRoomTop + targetH});
        rightWall.setScale({-(targetW / static_cast<float>(leftWallTexW)), wallGapH / static_cast<float>(leftWallTexH)});
        return;
    }

    const int hWallSourceW = 32;
    const int vWallSourceH = cornerSizeH - 64;

    for (int i = 0; i < 4; ++i) {
        auto& sprite = *room.m_cornerSprites[i];
        sprite.setTextureRect(sf::IntRect(sf::Vector2i(texX, texY), sf::Vector2i(cornerSizeW, cornerSizeH)));
        const bool flipX = (i % 2 == 1);
        const bool flipY = (i / 2 == 1);
        sprite.setOrigin(sf::Vector2f(0.0f, 0.0f));
        const float posX = Room::kRoomLeft + (flipX ? Room::kRoomWidth : 0.0f);
        const float posY = Room::kRoomTop + (flipY ? Room::kRoomHeight : 0.0f);
        sprite.setPosition(sf::Vector2f(posX, posY));
        sprite.setScale(sf::Vector2f((targetW / static_cast<float>(cornerSizeW)) * (flipX ? -1.0f : 1.0f),
                                     (targetH / static_cast<float>(cornerSizeH)) * (flipY ? -1.0f : 1.0f)));
    }

    for (int i = 0; i < 4; ++i) {
        auto& s = *room.m_wallSprites[i];
        if (i < 2) {
            s.setTextureRect(sf::IntRect(sf::Vector2i(texX + cornerSizeW, texY), sf::Vector2i(hWallSourceW, cornerSizeH)));
            s.setOrigin(sf::Vector2f(0.0f, 0.0f));
            const bool bottom = (i == 1);
            s.setPosition(sf::Vector2f(Room::kRoomLeft + targetW, Room::kRoomTop + (bottom ? Room::kRoomHeight : 0.0f)));
            s.setScale(sf::Vector2f(wallGapW / static_cast<float>(hWallSourceW),
                                    (targetH / static_cast<float>(cornerSizeH)) * (bottom ? -1.0f : 1.0f)));
        } else {
            s.setTextureRect(sf::IntRect(sf::Vector2i(texX, texY + 32), sf::Vector2i(32, vWallSourceH)));
            s.setOrigin(sf::Vector2f(0.0f, 0.0f));
            const bool right = (i == 3);
            s.setPosition(sf::Vector2f(Room::kRoomLeft + (right ? Room::kRoomWidth : 0.0f), Room::kRoomTop + targetH));
            s.setScale(sf::Vector2f((targetW / 32.0f) * (right ? -1.0f : 1.0f),
                                    wallGapH / static_cast<float>(vWallSourceH)));
        }
    }
}
