#pragma once

#include <cstddef>
#include <SFML/System/Vector2.hpp>

class Player;
class Room;

class RoomObjectsSystem {
public:
    static void breakProp(Room& room, std::size_t propIndex, const Player& player);
    static void collectPickup(Room& room, std::size_t pickupIndex, Player& player);
    static void collectReward(Room& room, Player& player);
    static void spawnRandomPickup(Room& room, const sf::Vector2f& position, float chance, const Player& player);
};
