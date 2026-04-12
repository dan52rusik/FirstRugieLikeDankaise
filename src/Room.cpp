#include "Room.h"

#include <algorithm>
#include <array>
#include <memory>
#include <random>

#include "Player.h"
#include "monsters/Boss.h"
#include "monsters/Fly.h"
#include "monsters/Knight.h"
#include "monsters/Spider.h"
#include "utils/Collision.h"

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

void Room::load(const RoomData& roomData) {
    for (int i = 0; i < 4; ++i) {
        m_doors[i] = roomData.doors[i];
    }
    m_roomType = roomData.type;
    m_cleared = roomData.cleared;
    buildRocks(roomData.layoutSeed);
    buildMonsters(roomData);
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

void Room::buildMonsters(const RoomData& roomData) {
    m_monsters.clear();

    if (roomData.cleared || roomData.type == RoomType::Start) {
        return;
    }

    std::mt19937 rng(static_cast<std::mt19937::result_type>(roomData.monsterSeed));
    std::uniform_int_distribution<int> typeDist(0, 2);

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
        default:
            m_monsters.emplace_back(std::make_unique<Knight>(spawn));
            break;
        }
    }
}

bool Room::isSpawnBlocked(const sf::Vector2f& position) const {
    const sf::FloatRect monsterBounds({position.x - 18.0f, position.y - 18.0f}, {36.0f, 36.0f});
    for (const auto& rock : m_rocks) {
        if (Collision::intersects(monsterBounds, rock.getGlobalBounds())) {
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
    }

    m_monsters.erase(
        std::remove_if(m_monsters.begin(), m_monsters.end(), [](const std::unique_ptr<Monster>& monster) {
            return !monster->isAlive();
        }),
        m_monsters.end());

    m_cleared = m_monsters.empty();
}

void Room::draw(sf::RenderTarget& target) const {
    target.draw(m_floor);
    target.draw(m_innerBounds);
    for (const auto& rock : m_rocks) {
        target.draw(rock);
    }
    for (int i = 0; i < 4; ++i) {
        if (m_doors[i]) {
            drawDoor(target, static_cast<Direction>(i));
        }
    }
    for (const auto& monster : m_monsters) {
        monster->draw(target);
    }
}

bool Room::collidesWithWalls(const sf::FloatRect& bounds) const {
    const sf::FloatRect roomBounds({kGridLeft, kGridTop}, {kGridCols * kTileSize, kGridRows * kTileSize});

    // Если вышли за пределы комнаты — проверяем, не в дверном проёме ли мы
    const bool inside = bounds.position.x >= roomBounds.position.x &&
                        bounds.position.y >= roomBounds.position.y &&
                        bounds.position.x + bounds.size.x <= roomBounds.position.x + roomBounds.size.x &&
                        bounds.position.y + bounds.size.y <= roomBounds.position.y + roomBounds.size.y;

    if (!inside) {
        // Разрешаем выход через открытые дверные проёмы
        if (m_cleared) {
            for (int i = 0; i < 4; ++i) {
                if (m_doors[i] && Collision::intersects(bounds, getDoorOpening(static_cast<Direction>(i)))) {
                    return false; // проём открыт — пропускаем
                }
            }
        }
        return true;
    }

    // Проверка внутренней зоны стен
    const bool nearTop    = bounds.position.y <= roomBounds.position.y + kWallThickness;
    const bool nearBottom = bounds.position.y + bounds.size.y >= roomBounds.position.y + roomBounds.size.y - kWallThickness;
    const bool nearLeft   = bounds.position.x <= roomBounds.position.x + kWallThickness;
    const bool nearRight  = bounds.position.x + bounds.size.x >= roomBounds.position.x + roomBounds.size.x - kWallThickness;

    if (nearTop || nearBottom || nearLeft || nearRight) {
        // Если комната очищена — проверяем, не стоим ли мы в дверном проёме
        if (m_cleared) {
            for (int i = 0; i < 4; ++i) {
                if (m_doors[i] && Collision::intersects(bounds, getDoorOpening(static_cast<Direction>(i)))) {
                    // В проёме двери — стена не блокирует
                    goto checkRocks;
                }
            }
        }
        return true;
    }

    checkRocks:
    for (const auto& rock : m_rocks) {
        if (Collision::intersects(bounds, rock.getGlobalBounds())) {
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
    const bool open = canUseDoor(direction);
    door.setFillColor(open ? sf::Color(190, 160, 95) : sf::Color(90, 60, 40));

    switch (direction) {
    case Direction::Up:
        door.setSize({kTileSize, kDoorThickness});
        door.setPosition({tileCenter(7, 0).x - kTileSize * 0.5f, kGridTop - kDoorThickness});
        break;
    case Direction::Down:
        door.setSize({kTileSize, kDoorThickness});
        door.setPosition({tileCenter(7, 8).x - kTileSize * 0.5f, kGridTop + kGridRows * kTileSize});
        break;
    case Direction::Left:
        door.setSize({kDoorThickness, kTileSize});
        door.setPosition({kGridLeft - kDoorThickness, tileCenter(0, 4).y - kTileSize * 0.5f});
        break;
    case Direction::Right:
        door.setSize({kDoorThickness, kTileSize});
        door.setPosition({kGridLeft + kGridCols * kTileSize, tileCenter(14, 4).y - kTileSize * 0.5f});
        break;
    }

    target.draw(door);
}
