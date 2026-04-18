#pragma once

namespace sf {
class RenderTarget;
}

enum class Direction;
class Room;

class RoomRenderer {
public:
    static void draw(const Room& room, sf::RenderTarget& target);

private:
    static void drawDoor(const Room& room, sf::RenderTarget& target, Direction direction);
};
