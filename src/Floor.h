#pragma once

#include <SFML/Graphics.hpp>
#include <map>
#include <vector>

enum class RoomType {
    Start,
    Normal,
    Boss
};

enum class Direction {
    Up,
    Down,
    Left,
    Right
};

struct RoomData {
    sf::Vector2i gridPosition;
    RoomType type{RoomType::Normal};
    bool visited{false};
    bool cleared{false};
    bool doors[4]{false, false, false, false};
    int layoutSeed{0};
    int monsterSeed{0};
};

class Floor {
public:
    Floor();

    void generate();
    RoomData& getCurrentRoom();
    const RoomData& getCurrentRoom() const;
    const std::map<int, RoomData>& getRooms() const;
    sf::Vector2i getCurrentGridPosition() const;
    bool tryMove(Direction direction);
    void markCurrentRoomCleared();

private:
    static int keyFromGrid(const sf::Vector2i& position);
    static sf::Vector2i directionOffset(Direction direction);
    static Direction opposite(Direction direction);

    std::map<int, RoomData> m_rooms;
    sf::Vector2i m_currentGridPosition;
};
