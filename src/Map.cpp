#include "Map.h"

#include <algorithm>
#include <array>
#include <vector>

namespace {
sf::Color roomColor(const RoomData& room, const Floor& floor) {
    sf::Color fill(130, 130, 130);
    if (room.visited) {
        fill = sf::Color(220, 220, 220);
    }
    if (room.type == RoomType::Boss) {
        fill = room.visited ? sf::Color(210, 98, 98) : sf::Color(180, 70, 70);
    } else if (room.type == RoomType::Treasure) {
        fill = room.visited ? sf::Color(242, 214, 96) : sf::Color(188, 148, 56);
    }
    if (room.gridPosition == floor.getCurrentGridPosition()) {
        fill = sf::Color(245, 220, 80);
    }
    return fill;
}
}

void Map::drawMiniMap(sf::RenderTarget& target, const Floor& floor, bool expanded) const {
    const sf::Vector2f panelSize = expanded ? sf::Vector2f(300.0f, 300.0f) : sf::Vector2f(136.0f, 136.0f);
    const sf::Vector2f panelPosition = expanded ? sf::Vector2f(624.0f, 210.0f) : sf::Vector2f(772.0f, 38.0f);
    const float cellSize = expanded ? 28.0f : 18.0f;
    const float spacing = expanded ? 34.0f : 22.0f;

    std::vector<const RoomData*> visibleRooms;
    visibleRooms.reserve(floor.getRooms().size());
    for (const auto& [_, room] : floor.getRooms()) {
        if (!room.visited && room.type != RoomType::Start) {
            continue;
        }
        visibleRooms.push_back(&room);
    }

    sf::RectangleShape panel(panelSize);
    panel.setPosition(panelPosition);
    panel.setFillColor(sf::Color(12, 12, 14, 185));
    panel.setOutlineColor(sf::Color(88, 88, 92, 220));
    panel.setOutlineThickness(2.0f);
    target.draw(panel);

    if (visibleRooms.empty()) {
        return;
    }

    int minX = visibleRooms.front()->gridPosition.x;
    int maxX = visibleRooms.front()->gridPosition.x;
    int minY = visibleRooms.front()->gridPosition.y;
    int maxY = visibleRooms.front()->gridPosition.y;
    for (const RoomData* room : visibleRooms) {
        minX = std::min(minX, room->gridPosition.x);
        maxX = std::max(maxX, room->gridPosition.x);
        minY = std::min(minY, room->gridPosition.y);
        maxY = std::max(maxY, room->gridPosition.y);
    }

    const float mapWidth = static_cast<float>(maxX - minX) * spacing + cellSize;
    const float mapHeight = static_cast<float>(maxY - minY) * spacing + cellSize;
    const sf::Vector2f anchor(
        panelPosition.x + (panelSize.x - mapWidth) * 0.5f,
        panelPosition.y + (panelSize.y - mapHeight) * 0.5f);

    for (const RoomData* room : visibleRooms) {
        sf::RectangleShape cell({cellSize, cellSize});
        const float cellX = anchor.x + static_cast<float>(room->gridPosition.x - minX) * spacing;
        const float cellY = anchor.y + static_cast<float>(room->gridPosition.y - minY) * spacing;
        cell.setPosition({cellX, cellY});

        const sf::Color fill = roomColor(*room, floor);
        cell.setFillColor(fill);
        cell.setOutlineColor(sf::Color::Black);
        cell.setOutlineThickness(1.0f);
        target.draw(cell);

        const float centerX = cellX + cellSize * 0.5f;
        const float centerY = cellY + cellSize * 0.5f;
        const float corridorShort = expanded ? 8.0f : 6.0f;
        const float corridorLong = expanded ? 6.0f : 4.0f;
        for (int d = 0; d < 4; ++d) {
            if (!room->doors[d]) {
                continue;
            }

            sf::RectangleShape corridor;
            corridor.setFillColor(fill);

            switch (static_cast<Direction>(d)) {
            case Direction::Up:
                corridor.setSize({corridorShort, corridorLong});
                corridor.setPosition({centerX - corridorShort * 0.5f, cellY - corridorLong});
                break;
            case Direction::Down:
                corridor.setSize({corridorShort, corridorLong});
                corridor.setPosition({centerX - corridorShort * 0.5f, cellY + cellSize});
                break;
            case Direction::Left:
                corridor.setSize({corridorLong, corridorShort});
                corridor.setPosition({cellX - corridorLong, centerY - corridorShort * 0.5f});
                break;
            case Direction::Right:
                corridor.setSize({corridorLong, corridorShort});
                corridor.setPosition({cellX + cellSize, centerY - corridorShort * 0.5f});
                break;
            }

            target.draw(corridor);
        }
    }
}
