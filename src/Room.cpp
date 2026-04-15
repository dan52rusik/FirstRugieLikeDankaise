#include "Room.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <memory>
#include <random>
#include <stdexcept>
#include <iostream>

#include "Player.h"
#include "monsters/Boss.h"
#include "monsters/Fly.h"
#include "monsters/Knight.h"
#include "monsters/Leech.h"
#include "monsters/Spider.h"
#include "rooms/RoomTemplateLoader.h"
#include "utils/Collision.h"
#include "utils/Random.h"

namespace {
#if SFML_VERSION_MAJOR < 3
float rectLeft(const sf::FloatRect& rect) { return rect.left; }
float rectTop(const sf::FloatRect& rect) { return rect.top; }
float rectWidth(const sf::FloatRect& rect) { return rect.width; }
float rectHeight(const sf::FloatRect& rect) { return rect.height; }
sf::FloatRect makeRect(const sf::Vector2f& position, const sf::Vector2f& size) { return {position.x, position.y, size.x, size.y}; }
int intRectWidth(const sf::IntRect& rect) { return rect.width; }
int intRectHeight(const sf::IntRect& rect) { return rect.height; }
void setRotationDegrees(sf::Transformable& transformable, float degrees) { transformable.setRotation(degrees); }
#else
float rectLeft(const sf::FloatRect& rect) { return rect.position.x; }
float rectTop(const sf::FloatRect& rect) { return rect.position.y; }
float rectWidth(const sf::FloatRect& rect) { return rect.size.x; }
float rectHeight(const sf::FloatRect& rect) { return rect.size.y; }
sf::FloatRect makeRect(const sf::Vector2f& position, const sf::Vector2f& size) { return {position, size}; }
int intRectWidth(const sf::IntRect& rect) { return rect.size.x; }
int intRectHeight(const sf::IntRect& rect) { return rect.size.y; }
void setRotationDegrees(sf::Transformable& transformable, float degrees) { transformable.setRotation(sf::degrees(degrees)); }
#endif

sf::Vector2f roomCenter() {
    return {480.0f, 300.0f};
}

sf::Vector2f tileCenter(int col, int row) {
    return {
        Room::kGridLeft + static_cast<float>(col) * Room::kTileSize + Room::kTileSize * 0.5f,
        Room::kGridTop + static_cast<float>(row) * Room::kTileSize + Room::kTileSize * 0.5f};
}

sf::Vector2f tileCenter(const sf::Vector2i& tile) {
    return tileCenter(tile.x, tile.y);
}

const std::array<sf::Vector2i, 8> kReservedDoorTiles{{
    {7, 0}, {7, 1}, {7, 7}, {7, 8},
    {0, 4}, {1, 4}, {13, 4}, {14, 4}
}};

bool isReservedDoorTile(const sf::Vector2i& tile) {
    return std::find(kReservedDoorTiles.begin(), kReservedDoorTiles.end(), tile) != kReservedDoorTiles.end();
}

bool isInsideGrid(const sf::Vector2i& tile) {
    return tile.x >= 0 && tile.x < Room::kGridCols && tile.y >= 0 && tile.y < Room::kGridRows;
}

sf::Vector2i tileFromWorld(const sf::Vector2f& position) {
    return {
        std::clamp(static_cast<int>((position.x - Room::kGridLeft) / Room::kTileSize), 0, Room::kGridCols - 1),
        std::clamp(static_cast<int>((position.y - Room::kGridTop) / Room::kTileSize), 0, Room::kGridRows - 1)
    };
}

void drawTexturedQuad(sf::RenderTarget& target,
                      const sf::Texture& texture,
                      const sf::IntRect& rect,
                      const std::array<sf::Vector2f, 4>& points,
                      float alpha,
                      bool mirrorX = false) {
    sf::VertexArray quad(sf::PrimitiveType::TriangleStrip, 4);

    const float left = static_cast<float>(rect.position.x);
    const float top = static_cast<float>(rect.position.y);
    const float right = left + static_cast<float>(intRectWidth(rect));
    const float bottom = top + static_cast<float>(intRectHeight(rect));
    const float texLeft = mirrorX ? right : left;
    const float texRight = mirrorX ? left : right;
    const sf::Color color(255, 255, 255, static_cast<std::uint8_t>(std::round(std::clamp(alpha, 0.0f, 1.0f) * 255.0f)));

    quad[0] = sf::Vertex{points[0], color, {texLeft, top}};
    quad[1] = sf::Vertex{points[1], color, {texRight, top}};
    quad[2] = sf::Vertex{points[2], color, {texLeft, bottom}};
    quad[3] = sf::Vertex{points[3], color, {texRight, bottom}};

    sf::RenderStates states;
    states.texture = &texture;
    target.draw(quad, states);
}
}

sf::Texture Room::s_floorTexture;
bool Room::s_floorTextureLoaded = false;
sf::Texture Room::s_backdropTexture;
bool Room::s_backdropTextureLoaded = false;
sf::Texture Room::s_wallTexture;
bool Room::s_wallTextureLoaded = false;
sf::Texture Room::s_doorTexture;
bool Room::s_doorTextureLoaded = false;

Room::Room() {
    if (!s_floorTextureLoaded) {
        const char* path = "assets/textures/rooms/01_Basement_nfloor.png";
        const char* fallback = "../assets/textures/rooms/01_Basement_nfloor.png";
        const char* fallback2 = "../../assets/textures/rooms/01_Basement_nfloor.png";

        s_floorTextureLoaded = s_floorTexture.loadFromFile(path);
        if (!s_floorTextureLoaded) s_floorTextureLoaded = s_floorTexture.loadFromFile(fallback);
        if (!s_floorTextureLoaded) s_floorTextureLoaded = s_floorTexture.loadFromFile(fallback2);
#ifdef __EMSCRIPTEN__
        if (!s_floorTextureLoaded) s_floorTextureLoaded = s_floorTexture.loadFromFile("/assets/textures/rooms/01_Basement_nfloor.png");
#endif
    }

    if (!s_backdropTextureLoaded) {
        const char* path = "assets/textures/rooms/01_Basement.png";
        const char* fallback = "../assets/textures/rooms/01_Basement.png";
        const char* fallback2 = "../../assets/textures/rooms/01_Basement.png";

        s_backdropTextureLoaded = s_backdropTexture.loadFromFile(path);
        if (!s_backdropTextureLoaded) s_backdropTextureLoaded = s_backdropTexture.loadFromFile(fallback);
        if (!s_backdropTextureLoaded) s_backdropTextureLoaded = s_backdropTexture.loadFromFile(fallback2);
#ifdef __EMSCRIPTEN__
        if (!s_backdropTextureLoaded) s_backdropTextureLoaded = s_backdropTexture.loadFromFile("/assets/textures/rooms/01_Basement.png");
#endif
    }

    if (!s_wallTextureLoaded) {
        const char* path = "assets/textures/rooms/Wall LVL1.png";
        const char* fallback = "../assets/textures/rooms/Wall LVL1.png";
        const char* fallback2 = "../../assets/textures/rooms/Wall LVL1.png";
        
        s_wallTextureLoaded = s_wallTexture.loadFromFile(path);
        if (!s_wallTextureLoaded) s_wallTextureLoaded = s_wallTexture.loadFromFile(fallback);
        if (!s_wallTextureLoaded) s_wallTextureLoaded = s_wallTexture.loadFromFile(fallback2);
        
        if (s_wallTextureLoaded) {
            sf::Vector2u size = s_wallTexture.getSize();
            std::cout << "Wall texture loaded: " << size.x << "x" << size.y << std::endl;
        }
#ifdef __EMSCRIPTEN__
        if (!s_wallTextureLoaded) s_wallTextureLoaded = s_wallTexture.loadFromFile("/assets/textures/rooms/Wall LVL1.png");
#endif
    }

    if (!s_doorTextureLoaded) {
        const char* path = "assets/textures/rooms/door_13_librarydoor_0.png";
        const char* fallback = "../assets/textures/rooms/door_13_librarydoor_0.png";
        const char* fallback2 = "../../assets/textures/rooms/door_13_librarydoor_0.png";

        s_doorTextureLoaded = s_doorTexture.loadFromFile(path);
        if (!s_doorTextureLoaded) s_doorTextureLoaded = s_doorTexture.loadFromFile(fallback);
        if (!s_doorTextureLoaded) s_doorTextureLoaded = s_doorTexture.loadFromFile(fallback2);
#ifdef __EMSCRIPTEN__
        if (!s_doorTextureLoaded) s_doorTextureLoaded = s_doorTexture.loadFromFile("/assets/textures/rooms/door_13_librarydoor_0.png");
#endif
    }

    m_floor.setSize({kRoomWidth, kRoomHeight});
    m_floor.setPosition({kRoomLeft, kRoomTop});
    m_floor.setFillColor(sf::Color(30, 80, 150)); // ЯРКО СИНИЙ ДЛЯ ТЕСТА
 
    m_innerBounds.setSize(m_floor.getSize());
    m_innerBounds.setPosition(m_floor.getPosition());
    m_innerBounds.setFillColor(sf::Color::Transparent);
    m_innerBounds.setOutlineThickness(20.0f);
    m_innerBounds.setOutlineColor(sf::Color::White); // Белая рамка

    if (s_backdropTextureLoaded) {
        const sf::Vector2u backdropSize = s_backdropTexture.getSize();
        const int frameW = static_cast<int>(backdropSize.x / 2);
        const int frameH = static_cast<int>(backdropSize.y / 3);
        const int variant = 0;
        const int texX = (variant % 2) * frameW;
        const int texY = (variant / 2) * frameH;

        // Только центральная часть фрейма (пол), стены отрисовываются отдельными спрайтами
        const int cw = 57, ch = 57; // размер углов в Isaac backdrop
        const int floorTexW = frameW - 2 * cw;
        const int floorTexH = frameH - 2 * ch;

        m_backdropSprite.emplace(s_backdropTexture);
        m_backdropSprite->setTextureRect(sf::IntRect(sf::Vector2i(texX, texY), sf::Vector2i(frameW, frameH)));
        m_backdropSprite->setPosition({kRoomLeft, kRoomTop});
        m_backdropSprite->setScale({
            kRoomWidth / static_cast<float>(frameW),
            kRoomHeight / static_cast<float>(frameH)});
    }

    if (s_wallTextureLoaded) {
        for (int i = 0; i < 4; ++i) {
            m_cornerSprites[i].emplace(s_wallTexture);
        }
    }

    for (int i = 0; i < kGridCols * kGridRows; ++i) {
        m_grid[i] = TileContent::Empty;
    }
}

void Room::load(RoomData& roomData) {
    m_roomData = &roomData;
    m_template = &RoomTemplateLoader::pick(roomData.type, roomData.layoutSeed);
    m_reward.reset();
    m_collectedReward.reset();

    for (int i = 0; i < 4; ++i) {
        m_doors[i] = roomData.doors[i];
    }

    m_roomType = roomData.type;
    m_cleared = roomData.cleared || roomData.type == RoomType::Treasure || roomData.type == RoomType::Start;
    m_doorOpenProgress = m_cleared ? 1.0f : 0.0f;

    switch (m_roomType) {
    case RoomType::Treasure:
        m_floor.setFillColor(sf::Color(92, 72, 36));
        m_innerBounds.setOutlineColor(sf::Color(122, 92, 42));
        break;
    case RoomType::Boss:
        m_floor.setFillColor(sf::Color(82, 48, 42));
        m_innerBounds.setOutlineColor(sf::Color(60, 26, 22));
        break;
    default:
        m_floor.setFillColor(sf::Color(72, 54, 42));
        m_innerBounds.setOutlineColor(sf::Color(42, 28, 22));
        break;
    }

    for (int i = 0; i < kGridCols * kGridRows; ++i) {
        m_grid[i] = TileContent::Empty;
    }

    buildRocks();
    buildProps(roomData);
    rebuildPropInstances();
    buildMonsters(roomData);
    buildReward(roomData);
    rebuildPickupInstances();

    const sf::Texture* wallTexture = s_backdropTextureLoaded ? &s_backdropTexture : (s_wallTextureLoaded ? &s_wallTexture : nullptr);
    if (wallTexture != nullptr && m_cornerSprites[0].has_value()) {
        const sf::Vector2u texSize = wallTexture->getSize();
        const int frameW = static_cast<int>(texSize.x / 2);
        const int frameH = static_cast<int>(texSize.y / 3);
        const int variant = 0;
        const int texX = (variant % 2) * frameW;
        const int texY = (variant / 2) * frameH;

        const bool usingIsaacBackdrop = wallTexture == &s_backdropTexture;
        const int cornerSizeW = usingIsaacBackdrop ? 57 : 250;
        const int cornerSizeH = usingIsaacBackdrop ? 57 : 200;
        const float targetW = kGridLeft - kRoomLeft;
        const float targetH = kGridTop - kRoomTop;
        const float wallGapW = kRoomWidth - targetW * 2.0f;
        const float wallGapH = kRoomHeight - targetH * 2.0f;

        for (int i = 0; i < 4; ++i) {
            if (!m_wallSprites[i].has_value()) {
                m_wallSprites[i].emplace(*wallTexture);
            }
        }

        if (usingIsaacBackdrop) {
            m_floorSprite.reset();
            const int floorLeft = 57;
            const int floorTop = 57;
            const int floorTexW = 176;
            const int floorTexH = 86;

            m_backdropSprite.emplace(s_backdropTexture);
            m_backdropSprite->setTextureRect(sf::IntRect(
                sf::Vector2i(texX + floorLeft, texY + floorTop),
                sf::Vector2i(floorTexW, floorTexH)));
            m_backdropSprite->setPosition({kGridLeft, kGridTop});
            m_backdropSprite->setScale({
                (kGridCols * kTileSize) / static_cast<float>(floorTexW),
                (kGridRows * kTileSize) / static_cast<float>(floorTexH)});

            for (int i = 0; i < 4; ++i) {
                m_cornerSprites[i].emplace(s_backdropTexture);
                m_wallSprites[i].emplace(s_backdropTexture);
            }

            const int wallLeft = 57;
            const int wallTop = 0;
            const int topWallTexW = 176;
            const int topWallTexH = 57;
            const int leftWallTexW = 57;
            const int leftWallTexH = 86;

            for (int i = 0; i < 4; ++i) {
                auto& sprite = *m_cornerSprites[i];
                const bool rightSide = (i % 2 == 1);
                const bool bottomSide = (i / 2 == 1);
                const int cornerSourceX = texX;
                const int cornerSourceY = texY;
                sprite.setTextureRect(sf::IntRect(sf::Vector2i(cornerSourceX, cornerSourceY), sf::Vector2i(cornerSizeW, cornerSizeH)));
                sprite.setOrigin({0.0f, 0.0f});
                const float posX = kRoomLeft + (rightSide ? kRoomWidth : 0.0f);
                const float posY = kRoomTop + (bottomSide ? kRoomHeight : 0.0f);
                sprite.setPosition({posX, posY});
                sprite.setScale({
                    (targetW / static_cast<float>(cornerSizeW)) * (rightSide ? -1.0f : 1.0f),
                    (targetH / static_cast<float>(cornerSizeH)) * (bottomSide ? -1.0f : 1.0f)});
            }

            auto& topWall = *m_wallSprites[0];
            topWall.setTextureRect(sf::IntRect(sf::Vector2i(texX + wallLeft, texY + wallTop), sf::Vector2i(topWallTexW, topWallTexH)));
            topWall.setOrigin({0.0f, 0.0f});
            topWall.setPosition({kRoomLeft + targetW, kRoomTop});
            topWall.setScale({wallGapW / static_cast<float>(topWallTexW), targetH / static_cast<float>(topWallTexH)});

            auto& bottomWall = *m_wallSprites[1];
            bottomWall.setTextureRect(sf::IntRect(sf::Vector2i(texX + wallLeft, texY + wallTop), sf::Vector2i(topWallTexW, topWallTexH)));
            bottomWall.setOrigin({0.0f, 0.0f});
            bottomWall.setPosition({kRoomLeft + targetW, kRoomTop + kRoomHeight});
            bottomWall.setScale({wallGapW / static_cast<float>(topWallTexW), -(targetH / static_cast<float>(topWallTexH))});

            auto& leftWall = *m_wallSprites[2];
            leftWall.setTextureRect(sf::IntRect(sf::Vector2i(texX, texY + floorTop), sf::Vector2i(leftWallTexW, leftWallTexH)));
            leftWall.setOrigin({0.0f, 0.0f});
            leftWall.setPosition({kRoomLeft, kRoomTop + targetH});
            leftWall.setScale({targetW / static_cast<float>(leftWallTexW), wallGapH / static_cast<float>(leftWallTexH)});

            auto& rightWall = *m_wallSprites[3];
            rightWall.setTextureRect(sf::IntRect(sf::Vector2i(texX, texY + floorTop), sf::Vector2i(leftWallTexW, leftWallTexH)));
            rightWall.setOrigin({0.0f, 0.0f});
            rightWall.setPosition({kRoomLeft + kRoomWidth, kRoomTop + targetH});
            rightWall.setScale({-(targetW / static_cast<float>(leftWallTexW)), wallGapH / static_cast<float>(leftWallTexH)});

        } else {
            // Fallback (Wall LVL1) — старая логика с зеркалированием
            const int hWallSourceW = 32;
            const int vWallSourceH = cornerSizeH - 64;

            for (int i = 0; i < 4; ++i) {
                auto& sprite = *m_cornerSprites[i];
                sprite.setTextureRect(sf::IntRect(sf::Vector2i(texX, texY), sf::Vector2i(cornerSizeW, cornerSizeH)));
                const bool flipX = (i % 2 == 1);
                const bool flipY = (i / 2 == 1);
                sprite.setOrigin(sf::Vector2f(0.0f, 0.0f));
                const float posX = kRoomLeft + (flipX ? kRoomWidth : 0.0f);
                const float posY = kRoomTop + (flipY ? kRoomHeight : 0.0f);
                sprite.setPosition(sf::Vector2f(posX, posY));
                sprite.setScale(sf::Vector2f((targetW / static_cast<float>(cornerSizeW)) * (flipX ? -1.0f : 1.0f),
                                             (targetH / static_cast<float>(cornerSizeH)) * (flipY ? -1.0f : 1.0f)));
            }

            for (int i = 0; i < 4; ++i) {
                auto& s = *m_wallSprites[i];
                if (i < 2) {
                    s.setTextureRect(sf::IntRect(sf::Vector2i(texX + cornerSizeW, texY), sf::Vector2i(hWallSourceW, cornerSizeH)));
                    s.setOrigin(sf::Vector2f(0.0f, 0.0f));
                    const bool bottom = (i == 1);
                    s.setPosition(sf::Vector2f(kRoomLeft + targetW, kRoomTop + (bottom ? kRoomHeight : 0.0f)));
                    s.setScale(sf::Vector2f(wallGapW / static_cast<float>(hWallSourceW),
                                            (targetH / static_cast<float>(cornerSizeH)) * (bottom ? -1.0f : 1.0f)));
                } else {
                    s.setTextureRect(sf::IntRect(sf::Vector2i(texX, texY + 32), sf::Vector2i(32, vWallSourceH)));
                    s.setOrigin(sf::Vector2f(0.0f, 0.0f));
                    const bool right = (i == 3);
                    s.setPosition(sf::Vector2f(kRoomLeft + (right ? kRoomWidth : 0.0f), kRoomTop + targetH));
                    s.setScale(sf::Vector2f((targetW / 32.0f) * (right ? -1.0f : 1.0f),
                                            wallGapH / static_cast<float>(vWallSourceH)));
                }
            }
        }
    }
}

int Room::getGridIndex(const sf::Vector2i& tile) const {
    return tile.y * kGridCols + tile.x;
}

int Room::getGridIndex(const sf::Vector2f& position) const {
    return getGridIndex(tileFromWorld(position));
}

void Room::buildRocks() {
    m_rocks.clear();
    if (m_template == nullptr) {
        return;
    }

    for (const auto& tile : m_template->rocks) {
        sf::RectangleShape rock({kTileSize, kTileSize});
        rock.setOrigin({24.0f, 24.0f});
        rock.setPosition(tileCenter(tile));
        rock.setFillColor(sf::Color(120, 120, 120));
        m_rocks.push_back(rock);
        
        if (isInsideGrid(tile)) {
            m_grid[getGridIndex(tile)] = TileContent::Rock;
        }
    }
}

void Room::buildProps(RoomData& roomData) {
    if (roomData.propsGenerated) {
        return;
    }

    roomData.propsGenerated = true;
    roomData.props.clear();
    roomData.pickups.clear();

    if (roomData.type != RoomType::Normal || m_template == nullptr) {
        return;
    }

    for (const auto& tile : m_template->barrels) {
        roomData.props.push_back({PropType::Barrel, tileCenter(tile), false});
        if (isInsideGrid(tile)) {
            m_grid[getGridIndex(tile)] = TileContent::Prop;
        }
    }
}

void Room::buildMonsters(const RoomData& roomData) {
    m_monsters.clear();

    if (roomData.cleared || roomData.type == RoomType::Start || roomData.type == RoomType::Treasure) {
        return;
    }

    if (roomData.type == RoomType::Boss) {
        m_monsters.emplace_back(std::make_unique<Boss>(tileCenter(7, 2)));
        return;
    }

    if (m_template == nullptr) {
        return;
    }

    std::mt19937 rng(static_cast<std::mt19937::result_type>(roomData.monsterSeed));
    std::uniform_int_distribution<int> typeDist(0, 3);

    const int targetCount = std::min(static_cast<int>(m_template->monsterSpawns.size()), 2 + (roomData.monsterSeed % 3));
    for (int i = 0; i < targetCount; ++i) {
        const sf::Vector2f spawn = tileCenter(m_template->monsterSpawns[static_cast<std::size_t>(i)]);
        if (isSpawnBlocked(spawn)) {
            continue;
        }

        switch (typeDist(rng)) {
        case 0:
            m_monsters.emplace_back(std::make_unique<Fly>(spawn));
            break;
        case 1:
            m_monsters.emplace_back(std::make_unique<Spider>(spawn));
            break;
        case 2:
            m_monsters.emplace_back(std::make_unique<Knight>(spawn));
            break;
        default:
            m_monsters.emplace_back(std::make_unique<Leech>(spawn));
            break;
        }
    }
}

void Room::buildReward(RoomData& roomData) {
    m_reward.reset();
    if (roomData.type != RoomType::Treasure || roomData.rewardTaken || m_template == nullptr || !m_template->rewardTile.has_value()) {
        return;
    }

    const std::vector<Item> items = createDefaultItems();
    if (items.empty()) {
        return;
    }

    if (roomData.rewardIndex < 0 || roomData.rewardIndex >= static_cast<int>(items.size())) {
        roomData.rewardIndex = roomData.rewardSeed % static_cast<int>(items.size());
    }

    const sf::Vector2f position = tileCenter(*m_template->rewardTile);

    RewardInstance reward;
    reward.item = items[static_cast<std::size_t>(roomData.rewardIndex)];

    reward.pedestal.setSize({34.0f, 26.0f});
    reward.pedestal.setOrigin({17.0f, 13.0f});
    reward.pedestal.setPosition({position.x, position.y + 12.0f});
    reward.pedestal.setFillColor(sf::Color(108, 84, 58));
    reward.pedestal.setOutlineColor(sf::Color(168, 132, 78));
    reward.pedestal.setOutlineThickness(2.0f);

    reward.icon.setRadius(10.0f);
    reward.icon.setOrigin({10.0f, 10.0f});
    reward.icon.setPosition({position.x, position.y - 8.0f});

    switch (reward.item.effect) {
    case ItemEffect::TearRate:
        reward.icon.setFillColor(reward.item.amount < 0.0f ? sf::Color(178, 228, 255) : sf::Color(118, 148, 188));
        break;
    case ItemEffect::Damage:
        reward.icon.setFillColor(reward.item.amount > 0.0f ? sf::Color(224, 72, 78) : sf::Color(110, 52, 58));
        break;
    case ItemEffect::Speed:
        reward.icon.setFillColor(reward.item.amount > 0.0f ? sf::Color(118, 214, 128) : sf::Color(70, 118, 74));
        break;
    }

    m_reward = reward;
}

void Room::rebuildPropInstances() {
    m_props.clear();
    if (m_roomData == nullptr) {
        return;
    }

    for (std::size_t i = 0; i < m_roomData->props.size(); ++i) {
        const auto& prop = m_roomData->props[i];
        if (prop.destroyed || prop.type != PropType::Barrel) {
            continue;
        }

        sf::RectangleShape barrel({30.0f, 34.0f});
        barrel.setOrigin({15.0f, 17.0f});
        barrel.setPosition(prop.position);
        barrel.setFillColor(sf::Color(114, 82, 42));
        barrel.setOutlineColor(sf::Color(70, 42, 22));
        barrel.setOutlineThickness(2.0f);
        m_props.push_back({barrel, i});
    }
}

void Room::rebuildPickupInstances() {
    m_pickups.clear();
    if (m_roomData == nullptr) {
        return;
    }

    for (std::size_t i = 0; i < m_roomData->pickups.size(); ++i) {
        const auto& pickup = m_roomData->pickups[i];
        if (pickup.collected) {
            continue;
        }

        sf::CircleShape shape(8.0f);
        shape.setOrigin({8.0f, 8.0f});
        shape.setPosition(pickup.position);

        switch (pickup.type) {
        case PickupType::Coin:
            shape.setFillColor(sf::Color(236, 204, 82));
            break;
        case PickupType::Heart:
            shape.setFillColor(sf::Color(220, 54, 78));
            break;
        case PickupType::Key:
            shape.setFillColor(sf::Color(210, 210, 210));
            break;
        case PickupType::Bomb:
            shape.setFillColor(sf::Color(54, 54, 58));
            break;
        }

        m_pickups.push_back({shape, pickup.type, i});
    }
}

bool Room::isTileOccupied(const sf::Vector2i& tile) const {
    if (!isInsideGrid(tile) || isReservedDoorTile(tile)) {
        return true;
    }

    if (m_grid[getGridIndex(tile)] != TileContent::Empty) {
        return true;
    }

    const sf::Vector2f center = tileCenter(tile);
    for (const auto& pickup : m_pickups) {
        if (Collision::distance(pickup.shape.getPosition(), center) < 1.0f) {
            return true;
        }
    }
    if (m_reward.has_value() && Collision::distance(m_reward->pedestal.getPosition(), center) < 1.0f) {
        return true;
    }
    for (const auto& monster : m_monsters) {
        if (monster->isAlive() && Collision::distance(monster->getPosition(), center) < 1.0f) {
            return true;
        }
    }
    return false;
}

sf::Vector2i Room::findFreeDropTile(const sf::Vector2f& position) const {
    const sf::Vector2i base = tileFromWorld(position);
    std::vector<sf::Vector2i> candidates;
    candidates.reserve(static_cast<std::size_t>(kGridCols * kGridRows));

    for (int row = 0; row < kGridRows; ++row) {
        for (int col = 0; col < kGridCols; ++col) {
            candidates.push_back({col, row});
        }
    }

    std::sort(candidates.begin(), candidates.end(), [&base](const sf::Vector2i& a, const sf::Vector2i& b) {
        const int da = std::abs(a.x - base.x) + std::abs(a.y - base.y);
        const int db = std::abs(b.x - base.x) + std::abs(b.y - base.y);
        return da < db;
    });

    for (const auto& tile : candidates) {
        if (!isTileOccupied(tile)) {
            return tile;
        }
    }
    return base;
}

void Room::spawnRandomPickup(const sf::Vector2f& position, float chance, const Player& player) {
    if (m_roomData == nullptr || !Random::chance(chance)) {
        return;
    }

    const int roll = Random::rangeInt(0, 99);
    PickupType type = PickupType::Coin;
    if (roll < 45) {
        type = PickupType::Coin;
    } else if (roll < 63) {
        type = PickupType::Bomb;
    } else if (roll < 81) {
        type = PickupType::Key;
    } else {
        type = (player.getHp() < player.getMaxHp()) ? PickupType::Heart : PickupType::Coin;
    }

    const sf::Vector2i dropTile = findFreeDropTile(position);
    m_roomData->pickups.push_back({type, tileCenter(dropTile), false});
    rebuildPickupInstances();
}

void Room::breakProp(std::size_t propIndex, const Player& player) {
    if (m_roomData == nullptr || propIndex >= m_roomData->props.size()) {
        return;
    }

    auto& prop = m_roomData->props[propIndex];
    if (prop.destroyed) {
        return;
    }

    prop.destroyed = true;
    const sf::Vector2i tile = tileFromWorld(prop.position);
    if (isInsideGrid(tile)) {
        m_grid[getGridIndex(tile)] = TileContent::Empty;
    }
    
    rebuildPropInstances();
    spawnRandomPickup(prop.position, 0.55f, player);
}

void Room::collectPickup(std::size_t pickupIndex, Player& player) {
    if (m_roomData == nullptr || pickupIndex >= m_roomData->pickups.size()) {
        return;
    }

    auto& pickup = m_roomData->pickups[pickupIndex];
    if (pickup.collected) {
        return;
    }

    switch (pickup.type) {
    case PickupType::Coin:
        player.addCoins(1);
        break;
    case PickupType::Heart:
        if (player.getHp() >= player.getMaxHp()) {
            return;
        }
        player.heal(2);
        break;
    case PickupType::Key:
        player.addKeys(1);
        break;
    case PickupType::Bomb:
        player.addBombs(1);
        break;
    }

    pickup.collected = true;
    rebuildPickupInstances();
}

void Room::collectReward(Player& player) {
    if (m_roomData == nullptr || !m_reward.has_value() || m_roomData->rewardTaken) {
        return;
    }

    player.applyItem(m_reward->item);
    m_collectedReward = m_reward->item;
    m_roomData->rewardTaken = true;
    m_reward.reset();
}

bool Room::isSpawnBlocked(const sf::Vector2f& position) const {
    const sf::FloatRect monsterBounds({position.x - 18.0f, position.y - 18.0f}, {36.0f, 36.0f});

    for (const auto& rock : m_rocks) {
        if (Collision::intersects(monsterBounds, rock.getGlobalBounds())) {
            return true;
        }
    }
    for (const auto& prop : m_props) {
        if (Collision::intersects(monsterBounds, prop.shape.getGlobalBounds())) {
            return true;
        }
    }
    if (m_reward.has_value() && Collision::intersects(monsterBounds, m_reward->pedestal.getGlobalBounds())) {
        return true;
    }
    return false;
}

void Room::keepMonsterInPlayableArea(Monster& monster) const {
    const sf::FloatRect roomBounds = makeRect({kGridLeft, kGridTop}, {kGridCols * kTileSize, kGridRows * kTileSize});
    const sf::FloatRect bounds = monster.getBounds();
    const float halfWidth = rectWidth(bounds) * 0.5f;
    const float halfHeight = rectHeight(bounds) * 0.5f;
    const float margin = 8.0f;

    sf::Vector2f position = monster.getPosition();
    position.x = std::clamp(position.x,
                            rectLeft(roomBounds) + margin + halfWidth,
                            rectLeft(roomBounds) + rectWidth(roomBounds) - margin - halfWidth);
    position.y = std::clamp(position.y,
                            rectTop(roomBounds) + margin + halfHeight,
                            rectTop(roomBounds) + rectHeight(roomBounds) - margin - halfHeight);

    for (const auto& rock : m_rocks) {
        sf::FloatRect adjustedBounds = makeRect({position.x - halfWidth, position.y - halfHeight},
                                                {rectWidth(bounds), rectHeight(bounds)});
        if (!Collision::intersects(adjustedBounds, rock.getGlobalBounds())) {
            continue;
        }

        const sf::Vector2f push = Collision::normalize(Collision::subtract(position, rock.getPosition()));
        const sf::Vector2f fallback = (push.x == 0.0f && push.y == 0.0f) ? sf::Vector2f(1.0f, 0.0f) : push;
        position = Collision::add(position, Collision::scale(fallback, 16.0f));
    }

    for (const auto& prop : m_props) {
        sf::FloatRect adjustedBounds = makeRect({position.x - halfWidth, position.y - halfHeight},
                                                {rectWidth(bounds), rectHeight(bounds)});
        if (!Collision::intersects(adjustedBounds, prop.shape.getGlobalBounds())) {
            continue;
        }

        const sf::Vector2f push = Collision::normalize(Collision::subtract(position, prop.shape.getPosition()));
        const sf::Vector2f fallback = (push.x == 0.0f && push.y == 0.0f) ? sf::Vector2f(1.0f, 0.0f) : push;
        position = Collision::add(position, Collision::scale(fallback, 16.0f));
    }

    if (m_reward.has_value()) {
        sf::FloatRect adjustedBounds = makeRect({position.x - halfWidth, position.y - halfHeight},
                                                {rectWidth(bounds), rectHeight(bounds)});
        if (Collision::intersects(adjustedBounds, m_reward->pedestal.getGlobalBounds())) {
            const sf::Vector2f push = Collision::normalize(Collision::subtract(position, m_reward->pedestal.getPosition()));
            const sf::Vector2f fallback = (push.x == 0.0f && push.y == 0.0f) ? sf::Vector2f(1.0f, 0.0f) : push;
            position = Collision::add(position, Collision::scale(fallback, 16.0f));
        }
    }

    monster.setPosition(position);
}

Player& Room::getPlayer() const {
    if (!m_currentPlayer) {
        throw std::runtime_error("Attempted to access player before it was set in the current room.");
    }
    return *m_currentPlayer;
}

void Room::update(float dt, Player& player, std::vector<Tear>& tears, std::vector<Bomb>& bombs) {
    m_currentPlayer = &player;
    for (auto& monster : m_monsters) {
        if (!monster->isAlive()) {
            continue;
        }

        monster->update(dt, *this);
        keepMonsterInPlayableArea(*monster);
        if (Collision::intersects(monster->getBounds(), player.getBounds())) {
            player.takeDamage(static_cast<int>(monster->getDamage()));
        }
    }

    for (auto& tear : tears) {
        if (!tear.isAlive()) {
            continue;
        }

        tear.update(dt, *this);

        bool hitMonster = false;
        for (auto& monster : m_monsters) {
            if (!monster->isAlive()) {
                continue;
            }
            if (!Collision::intersects(tear.getBounds(), monster->getBounds())) {
                continue;
            }
            if (monster->blocksShotFrom(tear.getPosition())) {
                tear.destroy();
                break;
            }
            monster->takeDamage(tear.getDamage());
            tear.destroy();
            hitMonster = true;
            break;
        }

        if (hitMonster) {
            continue;
        }

        bool hitProp = false;
        for (std::size_t propIndex = 0; propIndex < m_props.size(); ++propIndex) {
            if (Collision::intersects(tear.getBounds(), m_props[propIndex].shape.getGlobalBounds())) {
                breakProp(m_props[propIndex].dataIndex, player);
                tear.destroy();
                hitProp = true;
                break;
            }
        }

        if (hitProp) {
            continue;
        }

        if (collidesWithWalls(tear.getBounds())) {
            tear.destroy();
        }
    }

    for (auto& bomb : bombs) {
        bomb.update(dt, *this);
        if (!bomb.consumeExplosion()) {
            continue;
        }

        if (Collision::distance(player.getPosition(), bomb.getPosition()) <= bomb.getRadius()) {
            player.takeDamage(2);
        }

        for (auto& monster : m_monsters) {
            if (!monster->isAlive()) {
                continue;
            }
            if (Collision::distance(monster->getPosition(), bomb.getPosition()) <= bomb.getRadius()) {
                monster->takeDamage(60.0f);
            }
        }

        std::vector<std::size_t> propsToBreak;
        for (const auto& prop : m_props) {
            if (Collision::distance(prop.shape.getPosition(), bomb.getPosition()) <= bomb.getRadius()) {
                propsToBreak.push_back(prop.dataIndex);
            }
        }
        for (std::size_t propIndex : propsToBreak) {
            breakProp(propIndex, player);
        }
    }

    std::vector<sf::Vector2f> defeatedMonsterPositions;
    for (const auto& monster : m_monsters) {
        if (!monster->isAlive()) {
            defeatedMonsterPositions.push_back(monster->getPosition());
        }
    }
    for (const auto& position : defeatedMonsterPositions) {
        spawnRandomPickup(position, 0.28f, player);
    }

    if (m_reward.has_value() && Collision::intersects(player.getBounds(), m_reward->icon.getGlobalBounds())) {
        collectReward(player);
    }

    for (std::size_t pickupIndex = 0; pickupIndex < m_pickups.size(); ++pickupIndex) {
        if (Collision::intersects(player.getBounds(), m_pickups[pickupIndex].shape.getGlobalBounds())) {
            collectPickup(m_pickups[pickupIndex].dataIndex, player);
            break;
        }
    }

    m_monsters.erase(
        std::remove_if(m_monsters.begin(), m_monsters.end(), [](const std::unique_ptr<Monster>& monster) {
            return !monster->isAlive();
        }),
        m_monsters.end());

    m_cleared = m_monsters.empty() || m_roomType == RoomType::Treasure || m_roomType == RoomType::Start;
    if (m_roomData != nullptr) {
        m_roomData->cleared = m_cleared;
    }

    const float targetProgress = m_cleared ? 1.0f : 0.0f;
    const float doorAnimSpeed = 2.8f;
    if (m_doorOpenProgress < targetProgress) {
        m_doorOpenProgress = std::min(targetProgress, m_doorOpenProgress + dt * doorAnimSpeed);
    } else if (m_doorOpenProgress > targetProgress) {
        m_doorOpenProgress = std::max(targetProgress, m_doorOpenProgress - dt * doorAnimSpeed);
    }
}

void Room::draw(sf::RenderTarget& target) const {
    // Всегда рисуем сплошной фон как подложку (цвет задаётся в load())
    target.draw(m_floor);

    // Поверх — текстура пола, если есть
    if (m_floorSprite.has_value()) {
        target.draw(*m_floorSprite);
    } else if (m_backdropSprite.has_value()) {
        target.draw(*m_backdropSprite);
    }

    if (m_cornerSprites[0].has_value()) {
        for (int i = 0; i < 4; ++i) {
            target.draw(*m_cornerSprites[i]);
        }

        for (int i = 0; i < 4; ++i) {
            if (m_wallSprites[i].has_value()) {
                target.draw(*m_wallSprites[i]);
            }
        }
    } else if (!m_floorSprite.has_value() && !m_backdropSprite.has_value()) {
        target.draw(m_innerBounds);
    }

    for (const auto& rock : m_rocks) {
        target.draw(rock);
    }
    for (const auto& prop : m_props) {
        target.draw(prop.shape);
    }
    if (m_reward.has_value()) {
        target.draw(m_reward->pedestal);
        target.draw(m_reward->icon);
    }

    for (int i = 0; i < 4; ++i) {
        if (m_doors[i]) {
            drawDoor(target, static_cast<Direction>(i));
        }
    }

    for (const auto& pickup : m_pickups) {
        target.draw(pickup.shape);
    }
    for (const auto& monster : m_monsters) {
        monster->draw(target);
    }
}

bool Room::isInDoorOpening(const sf::FloatRect& bounds) const {
    if (!m_cleared) {
        return false;
    }
    for (int i = 0; i < 4; ++i) {
        if (m_doors[i] && Collision::intersects(bounds, getDoorOpening(static_cast<Direction>(i)))) {
            return true;
        }
    }
    return false;
}

bool Room::collidesWithWalls(const sf::FloatRect& bounds) const {
    const sf::FloatRect roomBounds = makeRect({kGridLeft, kGridTop}, {kGridCols * kTileSize, kGridRows * kTileSize});

    const bool inside = rectLeft(bounds) >= rectLeft(roomBounds) &&
                        rectTop(bounds) >= rectTop(roomBounds) &&
                        rectLeft(bounds) + rectWidth(bounds) <= rectLeft(roomBounds) + rectWidth(roomBounds) &&
                        rectTop(bounds) + rectHeight(bounds) <= rectTop(roomBounds) + rectHeight(roomBounds);

    if (!inside) {
        return !isInDoorOpening(bounds);
    }

    const bool nearTop = rectTop(bounds) <= rectTop(roomBounds) + kCollisionWallPadding;
    const bool nearBottom = rectTop(bounds) + rectHeight(bounds) >= rectTop(roomBounds) + rectHeight(roomBounds) - kCollisionWallPadding;
    const bool nearLeft = rectLeft(bounds) <= rectLeft(roomBounds) + kCollisionWallPadding;
    const bool nearRight = rectLeft(bounds) + rectWidth(bounds) >= rectLeft(roomBounds) + rectWidth(roomBounds) - kCollisionWallPadding;

    if ((nearTop || nearBottom || nearLeft || nearRight) && !isInDoorOpening(bounds)) {
        return true;
    }

    // Grid-based static collision
    const sf::Vector2f center(rectLeft(bounds) + rectWidth(bounds) * 0.5f, rectTop(bounds) + rectHeight(bounds) * 0.5f);
    const sf::Vector2i tile = tileFromWorld(center);
    if (m_grid[getGridIndex(tile)] != TileContent::Empty) {
        return true;
    }

    if (m_reward.has_value() && Collision::intersects(bounds, m_reward->pedestal.getGlobalBounds())) {
        return true;
    }
    return false;
}

bool Room::isCleared() const {
    return m_cleared;
}

bool Room::canUseDoor(Direction direction) const {
    return m_cleared && m_doors[static_cast<int>(direction)];
}

bool Room::hasBoss() const {
    if (m_roomType != RoomType::Boss) {
        return false;
    }
    for (const auto& monster : m_monsters) {
        if (monster->isBoss() && monster->isAlive()) {
            return true;
        }
    }
    return false;
}

float Room::getBossHpRatio() const {
    for (const auto& monster : m_monsters) {
        if (monster->isBoss()) {
            const float maxHp = monster->getMaxHp();
            return maxHp > 0.0f ? std::clamp(monster->getHp() / maxHp, 0.0f, 1.0f) : 0.0f;
        }
    }
    return 0.0f;
}

std::optional<Item> Room::consumeCollectedReward() {
    std::optional<Item> reward = m_collectedReward;
    m_collectedReward.reset();
    return reward;
}

sf::Vector2f Room::getSpawnPosition(Direction fromDirection) const {
    switch (fromDirection) {
    case Direction::Up:
        return tileCenter(7, 0);
    case Direction::Down:
        return tileCenter(7, 8);
    case Direction::Left:
        return tileCenter(0, 4);
    case Direction::Right:
        return tileCenter(14, 4);
    }
    return roomCenter();
}

sf::Vector2f Room::findSafePlayerSpawn(Direction fromDirection) const {
    const sf::Vector2f base = getSpawnPosition(fromDirection);
    const std::array<sf::Vector2f, 9> offsets{{
        {0.0f, 0.0f}, {0.0f, 48.0f}, {0.0f, -48.0f}, {48.0f, 0.0f}, {-48.0f, 0.0f},
        {48.0f, 48.0f}, {-48.0f, 48.0f}, {48.0f, -48.0f}, {-48.0f, -48.0f}
    }};

    for (const auto& offset : offsets) {
        const sf::Vector2f candidate = Collision::add(base, offset);
        const sf::FloatRect bounds({candidate.x - 14.0f, candidate.y - 14.0f}, {28.0f, 28.0f});
        if (!collidesWithWalls(bounds)) {
            return candidate;
        }
    }

    return roomCenter();
}

Direction Room::getDoorTransition(const sf::Vector2f& playerPosition) const {
    for (int i = 0; i < 4; ++i) {
        const Direction direction = static_cast<Direction>(i);
        if (canUseDoor(direction) && Collision::intersects(sf::FloatRect(playerPosition, {1.0f, 1.0f}), getDoorTrigger(direction))) {
            return direction;
        }
    }
    return Direction::Up;
}

bool Room::hasTransitionAt(const sf::Vector2f& playerPosition) const {
    for (int i = 0; i < 4; ++i) {
        const Direction direction = static_cast<Direction>(i);
        if (canUseDoor(direction) && Collision::intersects(sf::FloatRect(playerPosition, {1.0f, 1.0f}), getDoorTrigger(direction))) {
            return true;
        }
    }
    return false;
}

sf::FloatRect Room::getDoorOpening(Direction direction) const {
    const float margin = kWallThickness + 16.0f;

    switch (direction) {
    case Direction::Up:
        return {{tileCenter(7, 0).x - kTileSize * 0.5f, kGridTop - margin}, {kTileSize, margin * 2.0f}};
    case Direction::Down:
        return {{tileCenter(7, 8).x - kTileSize * 0.5f, kGridTop + kGridRows * kTileSize - margin}, {kTileSize, margin * 2.0f}};
    case Direction::Left:
        return {{kGridLeft - margin, tileCenter(0, 4).y - kTileSize * 0.5f}, {margin * 2.0f, kTileSize}};
    case Direction::Right:
        return {{kGridLeft + kGridCols * kTileSize - margin, tileCenter(14, 4).y - kTileSize * 0.5f}, {margin * 2.0f, kTileSize}};
    }
    return {{0.0f, 0.0f}, {0.0f, 0.0f}};
}

sf::FloatRect Room::getDoorTrigger(Direction direction) const {
    switch (direction) {
    case Direction::Up:
        return {{tileCenter(7, 0).x - kTileSize * 0.5f, kGridTop - 20.0f}, {kTileSize, 40.0f}};
    case Direction::Down:
        return {{tileCenter(7, 8).x - kTileSize * 0.5f, kGridTop + kGridRows * kTileSize - 20.0f}, {kTileSize, 40.0f}};
    case Direction::Left:
        return {{kGridLeft - 20.0f, tileCenter(0, 4).y - kTileSize * 0.5f}, {40.0f, kTileSize}};
    case Direction::Right:
        return {{kGridLeft + kGridCols * kTileSize - 20.0f, tileCenter(14, 4).y - kTileSize * 0.5f}, {40.0f, kTileSize}};
    }
    return {{0.0f, 0.0f}, {0.0f, 0.0f}};
}

void Room::drawDoor(sf::RenderTarget& target, Direction direction) const {
    const float t = std::clamp(m_doorOpenProgress, 0.0f, 1.0f);
    if (s_doorTextureLoaded) {
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
            const float frameWidth = 64.0f;
            const float frameHeight = 42.0f;
            const float fillWidth = 32.0f;
            const float fillHeight = 26.0f;
            const sf::Vector2f doorPosition = bottom
                ? sf::Vector2f(tileCenter(7, 8).x - frameWidth * 0.5f, kGridTop + kGridRows * kTileSize + frameHeight)
                : sf::Vector2f(tileCenter(7, 0).x - frameWidth * 0.5f, kGridTop - frameHeight + 6.0f);

            sf::Sprite frameSprite(s_doorTexture);
            frameSprite.setTextureRect(kFrameTopRect);
            frameSprite.setPosition(doorPosition);
            frameSprite.setScale({frameWidth / static_cast<float>(intRectWidth(kFrameTopRect)),
                                  (frameHeight / static_cast<float>(intRectHeight(kFrameTopRect))) * (bottom ? -1.0f : 1.0f)});
            withAlpha(frameSprite, 1.0f);
            target.draw(frameSprite);

            sf::Sprite fillSprite(s_doorTexture);
            fillSprite.setTextureRect(kOpenFillTopRect);
            const float fillX = tileCenter(7, bottom ? 8 : 0).x - fillWidth * 0.5f;
            const float fillY = bottom ? (kGridTop + kGridRows * kTileSize + fillHeight + 1.0f) : (kGridTop - fillHeight + 6.0f);
            fillSprite.setPosition({fillX, fillY});
            fillSprite.setScale({fillWidth / static_cast<float>(intRectWidth(kOpenFillTopRect)),
                                 (fillHeight / static_cast<float>(intRectHeight(kOpenFillTopRect))) * (bottom ? -1.0f : 1.0f)});
            withAlpha(fillSprite, t);
            target.draw(fillSprite);

            sf::Sprite closedLeftSprite(s_doorTexture);
            closedLeftSprite.setTextureRect(kClosedLeftRect);
            closedLeftSprite.setOrigin({intRectWidth(kClosedLeftRect) * 0.5f, intRectHeight(kClosedLeftRect) * 0.5f});
            closedLeftSprite.setPosition({tileCenter(7, bottom ? 8 : 0).x - fillWidth * 0.2f, bottom ? (kGridTop + kGridRows * kTileSize + fillHeight * 0.55f) : (kGridTop - fillHeight * 0.45f)});
            closedLeftSprite.setScale({(fillWidth * 0.55f) / static_cast<float>(intRectWidth(kClosedLeftRect)),
                                       (fillHeight * 0.95f) / static_cast<float>(intRectHeight(kClosedLeftRect)) * (bottom ? -1.0f : 1.0f)});
            withAlpha(closedLeftSprite, 1.0f - t);
            target.draw(closedLeftSprite);

            sf::Sprite closedRightSprite(s_doorTexture);
            closedRightSprite.setTextureRect(kClosedRightRect);
            closedRightSprite.setOrigin({intRectWidth(kClosedRightRect) * 0.5f, intRectHeight(kClosedRightRect) * 0.5f});
            closedRightSprite.setPosition({tileCenter(7, bottom ? 8 : 0).x + fillWidth * 0.2f, bottom ? (kGridTop + kGridRows * kTileSize + fillHeight * 0.55f) : (kGridTop - fillHeight * 0.45f)});
            closedRightSprite.setScale({(fillWidth * 0.55f) / static_cast<float>(intRectWidth(kClosedRightRect)),
                                        (fillHeight * 0.95f) / static_cast<float>(intRectHeight(kClosedRightRect)) * (bottom ? -1.0f : 1.0f)});
            withAlpha(closedRightSprite, 1.0f - t);
            target.draw(closedRightSprite);
            return;
        }
        case Direction::Left:
        case Direction::Right: {
            const bool right = direction == Direction::Right;
            const float frameWidth = 42.0f;
            const float frameHeight = 64.0f;
            const float fillWidth = 26.0f;
            const float fillHeight = 32.0f;
            const float centerX = right
                ? (kGridLeft + kGridCols * kTileSize)
                : kGridLeft;
            const float centerY = tileCenter(right ? 14 : 0, 4).y;
            const float rotation = right ? 90.0f : -90.0f;

            sf::Sprite frameSprite(s_doorTexture);
            frameSprite.setTextureRect(kFrameTopRect);
            frameSprite.setOrigin({intRectWidth(kFrameTopRect) * 0.5f, intRectHeight(kFrameTopRect) * 0.5f});
            frameSprite.setPosition({centerX, centerY});
            setRotationDegrees(frameSprite, rotation);
            frameSprite.setScale({frameHeight / static_cast<float>(intRectWidth(kFrameTopRect)),
                                  frameWidth / static_cast<float>(intRectHeight(kFrameTopRect))});
            withAlpha(frameSprite, 1.0f);
            target.draw(frameSprite);

            sf::Sprite fillSprite(s_doorTexture);
            fillSprite.setTextureRect(kOpenFillTopRect);
            fillSprite.setOrigin({intRectWidth(kOpenFillTopRect) * 0.5f, intRectHeight(kOpenFillTopRect) * 0.5f});
            fillSprite.setPosition({centerX, centerY});
            setRotationDegrees(fillSprite, rotation);
            fillSprite.setScale({fillHeight / static_cast<float>(intRectWidth(kOpenFillTopRect)),
                                 fillWidth / static_cast<float>(intRectHeight(kOpenFillTopRect))});
            withAlpha(fillSprite, t);
            target.draw(fillSprite);

            const float leafOffset = fillHeight * 0.2f;
            const sf::Vector2f leftLeafPosition = right
                ? sf::Vector2f(centerX, centerY - leafOffset)
                : sf::Vector2f(centerX, centerY + leafOffset);
            const sf::Vector2f rightLeafPosition = right
                ? sf::Vector2f(centerX, centerY + leafOffset)
                : sf::Vector2f(centerX, centerY - leafOffset);

            sf::Sprite closedLeftSprite(s_doorTexture);
            closedLeftSprite.setTextureRect(kClosedLeftRect);
            closedLeftSprite.setOrigin({intRectWidth(kClosedLeftRect) * 0.5f, intRectHeight(kClosedLeftRect) * 0.5f});
            closedLeftSprite.setPosition(leftLeafPosition);
            setRotationDegrees(closedLeftSprite, rotation);
            closedLeftSprite.setScale({fillHeight * 0.55f / static_cast<float>(intRectWidth(kClosedLeftRect)),
                                       fillWidth * 0.95f / static_cast<float>(intRectHeight(kClosedLeftRect))});
            withAlpha(closedLeftSprite, 1.0f - t);
            target.draw(closedLeftSprite);

            sf::Sprite closedRightSprite(s_doorTexture);
            closedRightSprite.setTextureRect(kClosedRightRect);
            closedRightSprite.setOrigin({intRectWidth(kClosedRightRect) * 0.5f, intRectHeight(kClosedRightRect) * 0.5f});
            closedRightSprite.setPosition(rightLeafPosition);
            setRotationDegrees(closedRightSprite, rotation);
            closedRightSprite.setScale({fillHeight * 0.55f / static_cast<float>(intRectWidth(kClosedRightRect)),
                                        fillWidth * 0.95f / static_cast<float>(intRectHeight(kClosedRightRect))});
            withAlpha(closedRightSprite, 1.0f - t);
            target.draw(closedRightSprite);
            return;
        }
        }
    }

    sf::RectangleShape door;
    const auto blend = [t](std::uint8_t closed, std::uint8_t open) -> std::uint8_t {
        return static_cast<std::uint8_t>(std::round(static_cast<float>(closed) + (static_cast<float>(open) - static_cast<float>(closed)) * t));
    };

    const bool treasureTint = m_roomType == RoomType::Treasure;
    const sf::Color closedColor = treasureTint ? sf::Color(126, 92, 36) : sf::Color(90, 60, 40);
    const sf::Color openColor = treasureTint ? sf::Color(222, 184, 72) : sf::Color(190, 160, 95);
    door.setFillColor(sf::Color(blend(closedColor.r, openColor.r),
                                blend(closedColor.g, openColor.g),
                                blend(closedColor.b, openColor.b)));

    switch (direction) {
    case Direction::Up:
        door.setSize({kTileSize, kDoorThickness * (1.0f - 0.6f * t)});
        door.setPosition({tileCenter(7, 0).x - kTileSize * 0.5f, kGridTop - door.getSize().y});
        break;
    case Direction::Down:
        door.setSize({kTileSize, kDoorThickness * (1.0f - 0.6f * t)});
        door.setPosition({tileCenter(7, 8).x - kTileSize * 0.5f, kGridTop + kGridRows * kTileSize});
        break;
    case Direction::Left:
        door.setSize({kDoorThickness * (1.0f - 0.6f * t), kTileSize});
        door.setPosition({kGridLeft - door.getSize().x, tileCenter(0, 4).y - kTileSize * 0.5f});
        break;
    case Direction::Right:
        door.setSize({kDoorThickness * (1.0f - 0.6f * t), kTileSize});
        door.setPosition({kGridLeft + kGridCols * kTileSize, tileCenter(14, 4).y - kTileSize * 0.5f});
        break;
    }

    target.draw(door);
}
