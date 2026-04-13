#pragma once

#include <SFML/Graphics.hpp>

class Room;

enum class EntityType {
    Player,
    Monster,
    Tear,
    Bomb,
    Pickup,
    Prop
};

class Entity {
public:
    Entity(EntityType type) : m_type(type), m_alive(true) {}
    virtual ~Entity() = default;

    virtual void update(float dt, Room& room) = 0;
    virtual void draw(sf::RenderTarget& target) const = 0;

    virtual sf::FloatRect getBounds() const = 0;
    virtual sf::Vector2f getPosition() const = 0;
    virtual void setPosition(const sf::Vector2f& pos) = 0;

    bool isAlive() const { return m_alive; }
    void kill() { m_alive = false; }
    
    EntityType getType() const { return m_type; }

    virtual void takeDamage(float amount) {}

protected:
    EntityType m_type;
    bool m_alive;
};
