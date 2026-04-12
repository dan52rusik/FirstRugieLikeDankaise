#pragma once

#include <SFML/Graphics.hpp>
#include <memory>
#include <vector>

#include "Bomb.h"
#include "Floor.h"
#include "Tear.h"
#include "items/Pickup.h"
#include "monsters/Monster.h"
#include "props/Prop.h"

class Player;

class Room {
public:
    Room();

    void load(RoomData& roomData);
    void update(float dt, Player& player, std::vector<Tear>& tears, std::vector<Bomb>& bombs);
    void draw(sf::RenderTarget& target) const;

    bool collidesWithWalls(const sf::FloatRect& bounds) const;
    bool isCleared() const;
    bool canUseDoor(Direction direction) const;
    bool hasBoss() const;
    float getBossHpRatio() const;
    sf::Vector2f getSpawnPosition(Direction fromDirection) const;
    sf::Vector2f findSafePlayerSpawn(Direction fromDirection) const;
    Direction getDoorTransition(const sf::Vector2f& playerPosition) const;
    bool hasTransitionAt(const sf::Vector2f& playerPosition) const;

private:
    struct PropInstance {
        sf::RectangleShape shape;
        std::size_t dataIndex;
    };

    struct PickupInstance {
        sf::CircleShape shape;
        PickupType type;
        std::size_t dataIndex;
    };

    void buildRocks(int layoutSeed);
    void buildProps(RoomData& roomData);
    void buildMonsters(const RoomData& roomData);
    void rebuildPropInstances();
    void rebuildPickupInstances();
    void breakProp(std::size_t propIndex, const Player& player);
    void collectPickup(std::size_t pickupIndex, Player& player);
    void spawnRandomPickup(const sf::Vector2f& position, float chance, const Player& player);
    bool isSpawnBlocked(const sf::Vector2f& position) const;
    bool isInDoorOpening(const sf::FloatRect& bounds) const;
    void keepMonsterInPlayableArea(Monster& monster) const;
    sf::FloatRect getDoorOpening(Direction direction) const;
    sf::FloatRect getDoorTrigger(Direction direction) const;
    void drawDoor(sf::RenderTarget& target, Direction direction) const;

    sf::RectangleShape m_floor;
    sf::RectangleShape m_innerBounds;
    RoomData* m_roomData{nullptr};
    std::vector<sf::RectangleShape> m_rocks;
    std::vector<PropInstance> m_props;
    std::vector<PickupInstance> m_pickups;
    std::vector<std::unique_ptr<Monster>> m_monsters;
    bool m_doors[4]{false, false, false, false};
    RoomType m_roomType{RoomType::Normal};
    bool m_cleared{false};
    float m_doorOpenProgress{0.0f};
};
