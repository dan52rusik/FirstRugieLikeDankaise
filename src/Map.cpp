#include "Map.h"

#include <array>

void Map::drawMiniMap(sf::RenderTarget& target, const Floor& floor) const {
    const sf::Vector2f anchor(818.0f, 56.0f);
    const float spacing = 22.0f;

    sf::RectangleShape panel({118.0f, 118.0f});
    panel.setPosition({792.0f, 30.0f});
    panel.setFillColor(sf::Color(12, 12, 14, 185));
    panel.setOutlineColor(sf::Color(88, 88, 92, 220));
    panel.setOutlineThickness(2.0f);
    target.draw(panel);

    for (const auto& [_, room] : floor.getRooms()) {
        if (!room.visited && room.type != RoomType::Start) {
            continue;
        }

        sf::RectangleShape cell({18.0f, 18.0f});
        cell.setPosition({
            anchor.x + static_cast<float>(room.gridPosition.x) * spacing,
            anchor.y + static_cast<float>(room.gridPosition.y) * spacing});

        sf::Color fill(130, 130, 130);
        if (room.type == RoomType::Boss) {
            fill = sf::Color(180, 70, 70);
        }
        if (room.visited) {
            fill = sf::Color(220, 220, 220);
        }
        if (room.gridPosition == floor.getCurrentGridPosition()) {
            fill = sf::Color(245, 220, 80);
        }

        cell.setFillColor(fill);
        cell.setOutlineColor(sf::Color::Black);
        cell.setOutlineThickness(1.0f);
        target.draw(cell);

        for (int d = 0; d < 4; ++d) {
            if (!room.doors[d]) {
                continue;
            }

            sf::RectangleShape corridor;
            corridor.setFillColor(fill);
            const float x = anchor.x + static_cast<float>(room.gridPosition.x) * spacing;
            const float y = anchor.y + static_cast<float>(room.gridPosition.y) * spacing;

            switch (static_cast<Direction>(d)) {
            case Direction::Up:
                corridor.setSize({6.0f, 4.0f});
                corridor.setPosition({x + 6.0f, y - 4.0f});
                break;
            case Direction::Down:
                corridor.setSize({6.0f, 4.0f});
                corridor.setPosition({x + 6.0f, y + 18.0f});
                break;
            case Direction::Left:
                corridor.setSize({4.0f, 6.0f});
                corridor.setPosition({x - 4.0f, y + 6.0f});
                break;
            case Direction::Right:
                corridor.setSize({4.0f, 6.0f});
                corridor.setPosition({x + 18.0f, y + 6.0f});
                break;
            }

            target.draw(corridor);
        }
    }
}
