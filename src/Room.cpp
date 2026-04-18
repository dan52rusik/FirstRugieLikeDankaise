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
#include "rooms/RoomCombatSystem.h"
#include "rooms/RoomContentBuilder.h"
#include "rooms/RoomObjectsSystem.h"
#include "rooms/RoomRenderer.h"
#include "rooms/RoomVisualBuilder.h"
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

    RoomVisualBuilder::initialize(*this);

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

    RoomVisualBuilder::applyTheme(*this);

    for (int i = 0; i < kGridCols * kGridRows; ++i) {
        m_grid[i] = TileContent::Empty;
    }

    buildRocks();
    buildProps(roomData);
    rebuildPropInstances();
    buildMonsters(roomData);
    buildReward(roomData);
    rebuildPickupInstances();

    RoomVisualBuilder::buildVisuals(*this);
}

int Room::getGridIndex(const sf::Vector2i& tile) const {
    return tile.y * kGridCols + tile.x;
}

int Room::getGridIndex(const sf::Vector2f& position) const {
    return getGridIndex(tileFromWorld(position));
}

void Room::buildRocks() {
    RoomContentBuilder::buildRocks(*this);
}

void Room::buildProps(RoomData& roomData) {
    RoomContentBuilder::buildProps(*this, roomData);
}

void Room::buildMonsters(const RoomData& roomData) {
    RoomContentBuilder::buildMonsters(*this, roomData);
}

void Room::buildReward(RoomData& roomData) {
    RoomContentBuilder::buildReward(*this, roomData);
}

void Room::rebuildPropInstances() {
    RoomContentBuilder::rebuildPropInstances(*this);
}

void Room::rebuildPickupInstances() {
    RoomContentBuilder::rebuildPickupInstances(*this);
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
    RoomObjectsSystem::spawnRandomPickup(*this, position, chance, player);
}

void Room::breakProp(std::size_t propIndex, const Player& player) {
    RoomObjectsSystem::breakProp(*this, propIndex, player);
}

void Room::collectPickup(std::size_t pickupIndex, Player& player) {
    RoomObjectsSystem::collectPickup(*this, pickupIndex, player);
}

void Room::collectReward(Player& player) {
    RoomObjectsSystem::collectReward(*this, player);
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

const Player& Room::getPlayer() const {
    if (!m_currentPlayer) {
        throw std::runtime_error("Attempted to access player before it was set in the current room.");
    }
    return *m_currentPlayer;
}

void Room::update(float dt, Player& player, std::vector<Tear>& tears, std::vector<Bomb>& bombs) {
    RoomCombatSystem::update(*this, dt, player, tears, bombs);
}

void Room::draw(sf::RenderTarget& target) const {
    RoomRenderer::draw(*this, target);
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






