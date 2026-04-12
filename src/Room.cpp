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

struct LayoutTemplate {
    std::vector<sf::Vector2i> rocks;
    std::vector<sf::Vector2i> barrels;
    std::vector<sf::Vector2i> monsterSpawns;
    sf::Vector2i rewardTile;
};

sf::Vector2f tileCenter(int col, int row) {
    return {
        kGridLeft + static_cast<float>(col) * kTileSize + kTileSize * 0.5f,
        kGridTop + static_cast<float>(row) * kTileSize + kTileSize * 0.5f};
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
    return tile.x >= 0 && tile.x < kGridCols && tile.y >= 0 && tile.y < kGridRows;
}

sf::Vector2i tileFromWorld(const sf::Vector2f& position) {
    return {
        std::clamp(static_cast<int>((position.x - kGridLeft) / kTileSize), 0, kGridCols - 1),
        std::clamp(static_cast<int>((position.y - kGridTop) / kTileSize), 0, kGridRows - 1)
    };
}

const LayoutTemplate& layoutForSeed(int seed) {
    static const std::array<LayoutTemplate, 4> layouts{{
        {{{{2, 2}, {12, 2}, {2, 6}, {12, 6}}},
         {{{7, 2}, {4, 4}, {10, 4}}},
         {{{3, 2}, {7, 2}, {11, 2}, {3, 6}, {7, 6}, {11, 6}}},
         {7, 4}},
        {{{{4, 2}, {10, 2}, {4, 6}, {10, 6}}},
         {{{7, 1}, {3, 4}, {11, 4}}},
         {{{2, 2}, {7, 2}, {12, 2}, {2, 6}, {7, 6}, {12, 6}}},
         {7, 4}},
        {{{{7, 2}, {4, 4}, {10, 4}, {7, 6}}},
         {{{3, 2}, {11, 2}, {7, 7}}},
         {{{2, 3}, {12, 3}, {3, 6}, {7, 5}, {11, 6}, {7, 1}}},
         {7, 4}},
        {{{{3, 3}, {11, 3}, {5, 5}, {9, 5}}},
         {{{7, 2}, {3, 6}, {11, 6}}},
         {{{2, 2}, {7, 2}, {12, 2}, {2, 5}, {7, 6}, {12, 5}}},
         {7, 4}}
    }};

    return layouts[static_cast<std::size_t>(seed % static_cast<int>(layouts.size()))];
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
    m_reward.reset();

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

    buildRocks(roomData.layoutSeed);
    buildProps(roomData);
    rebuildPropInstances();
    buildMonsters(roomData);
    buildReward(roomData);
    rebuildPickupInstances();
}

void Room::buildRocks(int layoutSeed) {
    m_rocks.clear();

    const LayoutTemplate& layout = layoutForSeed(layoutSeed);
    for (const auto& tile : layout.rocks) {
        sf::RectangleShape rock({kTileSize, kTileSize});
        rock.setOrigin({24.0f, 24.0f});
        rock.setPosition(tileCenter(tile));
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

    const LayoutTemplate& layout = layoutForSeed(roomData.layoutSeed);
    for (const auto& tile : layout.barrels) {
        roomData.props.push_back({PropType::Barrel, tileCenter(tile), false});
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

    const LayoutTemplate& layout = layoutForSeed(roomData.layoutSeed);
    std::mt19937 rng(static_cast<std::mt19937::result_type>(roomData.monsterSeed));
    std::uniform_int_distribution<int> typeDist(0, 3);

    const int targetCount = std::min(static_cast<int>(layout.monsterSpawns.size()), 2 + (roomData.monsterSeed % 3));
    for (int i = 0; i < targetCount; ++i) {
        const sf::Vector2f spawn = tileCenter(layout.monsterSpawns[static_cast<std::size_t>(i)]);
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
    if (roomData.type != RoomType::Treasure || roomData.rewardTaken) {
        return;
    }

    const std::vector<Item> items = createDefaultItems();
    if (items.empty()) {
        return;
    }

    if (roomData.rewardIndex < 0 || roomData.rewardIndex >= static_cast<int>(items.size())) {
        roomData.rewardIndex = roomData.rewardSeed % static_cast<int>(items.size());
    }

    const LayoutTemplate& layout = layoutForSeed(roomData.layoutSeed);
    const sf::Vector2f position = tileCenter(layout.rewardTile);

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

    const sf::Vector2f center = tileCenter(tile);
    for (const auto& rock : m_rocks) {
        if (Collision::distance(rock.getPosition(), center) < 1.0f) {
            return true;
        }
    }
    for (const auto& prop : m_props) {
        if (Collision::distance(prop.shape.getPosition(), center) < 1.0f) {
            return true;
        }
    }
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

    if (m_reward.has_value()) {
        sf::FloatRect adjustedBounds({position.x - halfWidth, position.y - halfHeight}, bounds.size);
        if (Collision::intersects(adjustedBounds, m_reward->pedestal.getGlobalBounds())) {
            const sf::Vector2f push = Collision::normalize(Collision::subtract(position, m_reward->pedestal.getPosition()));
            const sf::Vector2f fallback = (push.x == 0.0f && push.y == 0.0f) ? sf::Vector2f(1.0f, 0.0f) : push;
            position = Collision::add(position, Collision::scale(fallback, 16.0f));
        }
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
    target.draw(m_floor);
    target.draw(m_innerBounds);

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
