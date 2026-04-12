#pragma once

#include <SFML/Graphics.hpp>
#include <memory>
#include <vector>

#include "Bomb.h"
#include "Floor.h"
#include "Tear.h"
#include "monsters/Monster.h"

class Player;

class Room {
public:
    Room();

    void load(const RoomData& roomData);
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
    void buildRocks(int layoutSeed);
    void buildMonsters(const RoomData& roomData);
    bool isSpawnBlocked(const sf::Vector2f& position) const;
    bool isInDoorOpening(const sf::FloatRect& bounds) const;
    void keepMonsterInPlayableArea(Monster& monster) const;
    sf::FloatRect getDoorOpening(Direction direction) const;
    sf::FloatRect getDoorTrigger(Direction direction) const;
    void drawDoor(sf::RenderTarget& target, Direction direction) const;

    sf::RectangleShape m_floor;
    sf::RectangleShape m_innerBounds;
    std::vector<sf::RectangleShape> m_rocks;
    std::vector<std::unique_ptr<Monster>> m_monsters;
    bool m_doors[4]{false, false, false, false};
    RoomType m_roomType{RoomType::Normal};
    bool m_cleared{false};
    float m_doorOpenProgress{0.0f};
};
