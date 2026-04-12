#include "Floor.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

#include "utils/Random.h"

Floor::Floor() {
    generate();
}

int Floor::keyFromGrid(const sf::Vector2i& position) {
    return position.x * 1000 + position.y;
}

sf::Vector2i Floor::directionOffset(Direction direction) {
    switch (direction) {
    case Direction::Up:
        return {0, -1};
    case Direction::Down:
        return {0, 1};
    case Direction::Left:
        return {-1, 0};
    case Direction::Right:
        return {1, 0};
    }
    return {0, 0};
}

void Floor::generate() {
    m_rooms.clear();
    m_currentGridPosition = {0, 0};

    RoomData startRoom;
    startRoom.gridPosition = {0, 0};
    startRoom.type = RoomType::Start;
    startRoom.visited = true;
    startRoom.cleared = true;
    startRoom.layoutSeed = Random::rangeInt(0, 100000);
    startRoom.monsterSeed = Random::rangeInt(0, 100000);
    m_rooms.emplace(keyFromGrid(startRoom.gridPosition), startRoom);

    const int targetRooms = Random::rangeInt(10, 15);
    std::vector<sf::Vector2i> frontier{startRoom.gridPosition};
    const std::array<Direction, 4> directions{Direction::Up, Direction::Down, Direction::Left, Direction::Right};

    while (static_cast<int>(m_rooms.size()) < targetRooms && !frontier.empty()) {
        const sf::Vector2i base = frontier[Random::rangeInt(0, static_cast<int>(frontier.size()) - 1)];
        std::vector<Direction> shuffled = {directions.begin(), directions.end()};
        std::shuffle(shuffled.begin(), shuffled.end(), Random::engine());

        bool createdRoom = false;
        for (Direction direction : shuffled) {
            const sf::Vector2i candidate = base + directionOffset(direction);
            if (m_rooms.count(keyFromGrid(candidate)) > 0) {
                continue;
            }

            RoomData room;
            room.gridPosition = candidate;
            room.type = RoomType::Normal;
            room.layoutSeed = Random::rangeInt(0, 100000);
            room.monsterSeed = Random::rangeInt(0, 100000);
            m_rooms.emplace(keyFromGrid(candidate), room);
            frontier.push_back(candidate);

            RoomData& from = m_rooms.at(keyFromGrid(base));
            RoomData& to = m_rooms.at(keyFromGrid(candidate));
            from.doors[static_cast<int>(direction)] = true;
            to.doors[static_cast<int>(oppositeDirection(direction))] = true;
            createdRoom = true;
            break;
        }

        if (!createdRoom) {
            frontier.erase(std::remove(frontier.begin(), frontier.end(), base), frontier.end());
        }
    }

    sf::Vector2i farthest = startRoom.gridPosition;
    int bestDistance = -1;
    for (const auto& [_, room] : m_rooms) {
        const int distance = std::abs(room.gridPosition.x) + std::abs(room.gridPosition.y);
        if (distance > bestDistance) {
            bestDistance = distance;
            farthest = room.gridPosition;
        }
    }

    m_rooms.at(keyFromGrid(farthest)).type = RoomType::Boss;
    m_rooms.at(keyFromGrid(startRoom.gridPosition)).type = RoomType::Start;
    m_rooms.at(keyFromGrid(startRoom.gridPosition)).cleared = true;
}

RoomData& Floor::getCurrentRoom() {
    return m_rooms.at(keyFromGrid(m_currentGridPosition));
}

const RoomData& Floor::getCurrentRoom() const {
    return m_rooms.at(keyFromGrid(m_currentGridPosition));
}

const std::map<int, RoomData>& Floor::getRooms() const {
    return m_rooms;
}

sf::Vector2i Floor::getCurrentGridPosition() const {
    return m_currentGridPosition;
}

bool Floor::tryMove(Direction direction) {
    RoomData& current = getCurrentRoom();
    if (!current.doors[static_cast<int>(direction)]) {
        return false;
    }

    const sf::Vector2i next = m_currentGridPosition + directionOffset(direction);
    auto it = m_rooms.find(keyFromGrid(next));
    if (it == m_rooms.end()) {
        return false;
    }

    m_currentGridPosition = next;
    it->second.visited = true;
    return true;
}

void Floor::markCurrentRoomCleared() {
    getCurrentRoom().cleared = true;
}
