#pragma once

#include <vector>

class Bomb;
class Player;
class Room;
class Tear;

class RoomCombatSystem {
public:
    static void update(Room& room, float dt, Player& player, std::vector<Tear>& tears, std::vector<Bomb>& bombs);
};
