#include "Room.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <memory>
#include <random>

#include "Player.h"
#include "monsters/Boss.h"
#include "monsters/Fly.h"
#include "monsters/Knight.h"
#include "monsters/Leech.h"
#include "monsters/Spider.h"
#include "utils/Collision.h"
#include "utils/Random.h"

namespace {
sf::Vector2f roomCenter() {
    return {480.0f, 300.0f};
}

constexpr float kRoomLeft = 72.0f;
constexpr float kRoomTop = 36.0f;
constexpr float kRoomWidth = 816.0f;
constexpr float kRoomHeight = 528.0f;
constexpr float kWallThickness = 24.0f;
constexpr float kTileSize = 48.0f;
constexpr int kGridCols = 15;
constexpr int kGridRows = 9;
constexpr float kGridLeft = kRoomLeft + 48.0f;
constexpr float kGridTop = kRoomTop + 48.0f;
constexpr float kDoorThickness = 18.0f;

sf::Vector2f tileCenter(int col, int row) {
    return {
        kGridLeft + static_cast<float>(col) * kTileSize + kTileSize * 0.5f,
        kGridTop + static_cast<float>(row) * kTileSize + kTileSize * 0.5f};
}
}

Room::Room() {
    m_floor.setSize({kRoomWidth, kRoomHeight});
    m_floor.setPosition({kRoomLeft, kRoomTop});
    m_floor.setFillColor(sf::Color(72, 54, 42));

    m_innerBounds.setSize(m_floor.getSize());
    m_innerBounds.setPosition(m_floor.getPosition());
    m_innerBounds.setFillColor(sf::Color::Transparent);
    m_innerBounds.setOutlineThickness(20.0f);
    m_innerBounds.setOutlineColor(sf::Color(42, 28, 22));
}

void Room::load(RoomData& roomData) {
    m_roomData = &roomData;
    for (int i = 0; i < 4; ++i) {
        m_doors[i] = roomData.doors[i];
    }
    m_roomType = roomData.type;
    m_cleared = roomData.cleared;
    m_doorOpenProgress = roomData.cleared ? 1.0f : 0.0f;
    buildRocks(roomData.layoutSeed);
    buildProps(roomData);
    rebuildPropInstances();
    buildMonsters(roomData);
    rebuildPickupInstances();
}

void Room::buildRocks(int layoutSeed) {
    m_rocks.clear();

    static const std::array<std::array<sf::Vector2i, 4>, 4> layouts{{
        {{{2, 2}, {12, 2}, {2, 6}, {12, 6}}},
        {{{4, 2}, {10, 2}, {4, 6}, {10, 6}}},
        {{{7, 2}, {4, 4}, {10, 4}, {7, 6}}},
        {{{3, 3}, {11, 3}, {5, 5}, {9, 5}}}
    }};

    const auto& layout = layouts[layoutSeed % static_cast<int>(layouts.size())];
    for (const auto& tile : layout) {
        sf::RectangleShape rock({kTileSize, kTileSize});
        rock.setOrigin({24.0f, 24.0f});
        rock.setPosition(tileCenter(tile.x, tile.y));
        rock.setFillColor(sf::Color(120, 120, 120));
        m_rocks.push_back(rock);
    }
}

void Room::buildProps(RoomData& roomData) {
    if (roomData.propsGenerated) {
        return;
    }

    roomData.propsGenerated = true;
    roomData.props.clear();
    roomData.pickups.clear();

    if (roomData.type != RoomType::Normal) {
        return;
    }

    static const std::array<std::array<sf::Vector2i, 3>, 4> barrelLayouts{{
        {{{7, 1}, {4, 4}, {10, 6}}},
        {{{3, 2}, {11, 4}, {7, 7}}},
        {{{5, 2}, {9, 2}, {7, 6}}},
        {{{3, 5}, {7, 3}, {11, 5}}}
    }};

    const auto& layout = barrelLayouts[roomData.layoutSeed % static_cast<int>(barrelLayouts.size())];
    for (const auto& tile : layout) {
        roomData.props.push_back({PropType::Barrel, tileCenter(tile.x, tile.y), false});
    }
}

void Room::buildMonsters(const RoomData& roomData) {
    m_monsters.clear();

    if (roomData.cleared || roomData.type == RoomType::Start) {
        return;
    }

    std::mt19937 rng(static_cast<std::mt19937::result_type>(roomData.monsterSeed));
    std::uniform_int_distribution<int> typeDist(0, 3);

    if (roomData.type == RoomType::Boss) {
        m_monsters.emplace_back(std::make_unique<Boss>(tileCenter(7, 2)));
        return;
    }

    const int monsterCount = 2 + (roomData.monsterSeed % 3);
    const std::array<sf::Vector2i, 8> spawnTiles{{
        {2, 1}, {7, 1}, {12, 1}, {3, 4}, {11, 4}, {2, 7}, {7, 7}, {12, 7}
    }};
    const std::array<sf::Vector2i, 8> reservedDoorTiles{{
        {7, 0}, {7, 1}, {7, 7}, {7, 8},
        {0, 4}, {1, 4}, {13, 4}, {14, 4}
    }};

    for (int i = 0; i < monsterCount; ++i) {
        const sf::Vector2i spawnTile = spawnTiles[static_cast<std::size_t>(i % static_cast<int>(spawnTiles.size()))];
        bool spawnNearDoor = false;
        for (const auto& reservedTile : reservedDoorTiles) {
            if (spawnTile == reservedTile) {
                spawnNearDoor = true;
                break;
            }
        }
        if (spawnNearDoor) {
            continue;
        }

        sf::Vector2f spawn = tileCenter(spawnTile.x, spawnTile.y);
        if (isSpawnBlocked(spawn)) {
            bool foundFreeSpawn = false;
            for (const auto& tile : spawnTiles) {
                const sf::Vector2f candidate = tileCenter(tile.x, tile.y);
                bool reserved = false;
                for (const auto& reservedTile : reservedDoorTiles) {
                    if (tile == reservedTile) {
                        reserved = true;
                        break;
                    }
                }

                if (!reserved && !isSpawnBlocked(candidate)) {
                    spawn = candidate;
                    foundFreeSpawn = true;
                    break;
                }
            }

            if (!foundFreeSpawn) {
                continue;
            }
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

void Room::spawnRandomPickup(const sf::Vector2f& position, float chance, const Player& player) {
    if (m_roomData == nullptr || !Random::chance(chance)) {
        return;
    }

    const int roll = Random::rangeInt(0, 99);
    PickupType type = PickupType::Coin;
    if (roll < 40) {
        type = PickupType::Coin;
    } else if (roll < 60) {
        type = PickupType::Bomb;
    } else if (roll < 80) {
        type = PickupType::Key;
    } else {
        type = (player.getHp() < player.getMaxHp()) ? PickupType::Heart : PickupType::Coin;
    }

    m_roomData->pickups.push_back({type, position, false});
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
    return false;
}

void Room::keepMonsterInPlayableArea(Monster& monster) const {
    const sf::FloatRect roomBounds({kGridLeft, kGridTop}, {kGridCols * kTileSize, kGridRows * kTileSize});
    const sf::FloatRect bounds = monster.getBounds();
    const float halfWidth = bounds.size.x * 0.5f;
    const float halfHeight = bounds.size.y * 0.5f;
    const float margin = 8.0f;

    sf::Vector2f position = monster.getPosition();
    position.x = std::clamp(position.x,
                            roomBounds.position.x + margin + halfWidth,
                            roomBounds.position.x + roomBounds.size.x - margin - halfWidth);
    position.y = std::clamp(position.y,
                            roomBounds.position.y + margin + halfHeight,
                            roomBounds.position.y + roomBounds.size.y - margin - halfHeight);

    for (const auto& rock : m_rocks) {
        sf::FloatRect adjustedBounds({position.x - halfWidth, position.y - halfHeight}, bounds.size);
        if (!Collision::intersects(adjustedBounds, rock.getGlobalBounds())) {
            continue;
        }

        const sf::Vector2f push = Collision::normalize(Collision::subtract(position, rock.getPosition()));
        const sf::Vector2f fallback = (push.x == 0.0f && push.y == 0.0f) ? sf::Vector2f(1.0f, 0.0f) : push;
        position = Collision::add(position, Collision::scale(fallback, 16.0f));
    }

    for (const auto& prop : m_props) {
        sf::FloatRect adjustedBounds({position.x - halfWidth, position.y - halfHeight}, bounds.size);
        if (!Collision::intersects(adjustedBounds, prop.shape.getGlobalBounds())) {
            continue;
        }

        const sf::Vector2f push = Collision::normalize(Collision::subtract(position, prop.shape.getPosition()));
        const sf::Vector2f fallback = (push.x == 0.0f && push.y == 0.0f) ? sf::Vector2f(1.0f, 0.0f) : push;
        position = Collision::add(position, Collision::scale(fallback, 16.0f));
    }

    monster.setPosition(position);
}

void Room::update(float dt, Player& player, std::vector<Tear>& tears, std::vector<Bomb>& bombs) {
    for (auto& monster : m_monsters) {
        if (!monster->isAlive()) {
            continue;
        }

        monster->update(dt, player, *this);
        keepMonsterInPlayableArea(*monster);
        if (Collision::intersects(monster->getBounds(), player.getBounds())) {
            player.takeDamage(static_cast<int>(monster->getDamage()));
        }
    }

    for (auto& tear : tears) {
        if (!tear.isAlive()) {
            continue;
        }

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
            continue;
        }
    }

    for (auto& bomb : bombs) {
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

    for (const auto& monster : m_monsters) {
        if (!monster->isAlive()) {
            spawnRandomPickup(monster->getPosition(), 0.28f, player);
        }
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

    m_cleared = m_monsters.empty();
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
    target.draw(m_floor);
    target.draw(m_innerBounds);
    for (const auto& rock : m_rocks) {
        target.draw(rock);
    }
    for (const auto& prop : m_props) {
        target.draw(prop.shape);
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
    const sf::FloatRect roomBounds({kGridLeft, kGridTop}, {kGridCols * kTileSize, kGridRows * kTileSize});

    const bool inside = bounds.position.x >= roomBounds.position.x &&
                        bounds.position.y >= roomBounds.position.y &&
                        bounds.position.x + bounds.size.x <= roomBounds.position.x + roomBounds.size.x &&
                        bounds.position.y + bounds.size.y <= roomBounds.position.y + roomBounds.size.y;

    if (!inside) {
        return !isInDoorOpening(bounds);
    }

    const bool nearTop = bounds.position.y <= roomBounds.position.y + kWallThickness;
    const bool nearBottom = bounds.position.y + bounds.size.y >= roomBounds.position.y + roomBounds.size.y - kWallThickness;
    const bool nearLeft = bounds.position.x <= roomBounds.position.x + kWallThickness;
    const bool nearRight = bounds.position.x + bounds.size.x >= roomBounds.position.x + roomBounds.size.x - kWallThickness;

    if ((nearTop || nearBottom || nearLeft || nearRight) && !isInDoorOpening(bounds)) {
        return true;
    }

    for (const auto& rock : m_rocks) {
        if (Collision::intersects(bounds, rock.getGlobalBounds())) {
            return true;
        }
    }
    for (const auto& prop : m_props) {
        if (Collision::intersects(bounds, prop.shape.getGlobalBounds())) {
            return true;
        }
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
    sf::RectangleShape door;
    const float t = std::clamp(m_doorOpenProgress, 0.0f, 1.0f);
    const auto blend = [t](std::uint8_t closed, std::uint8_t open) -> std::uint8_t {
        return static_cast<std::uint8_t>(std::round(static_cast<float>(closed) + (static_cast<float>(open) - static_cast<float>(closed)) * t));
    };
    door.setFillColor(sf::Color(blend(90, 190), blend(60, 160), blend(40, 95)));

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
