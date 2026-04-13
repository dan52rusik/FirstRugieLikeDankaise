#pragma once
#include "Entity.h"

class Tear : public Entity {
public:
    Tear(const sf::Vector2f& start, const sf::Vector2f& direction, float speed, float damage, float maxDistance);

    void update(float dt, Room& room) override;
    void draw(sf::RenderTarget& target) const override;

    // isAlive() is now in Entity
    void destroy();
    float getDamage() const;
    sf::FloatRect getBounds() const override;
    sf::Vector2f getPosition() const override;
    void setPosition(const sf::Vector2f& position) override;

private:
    sf::CircleShape m_shape;
    sf::Vector2f m_velocity;
    sf::Vector2f m_origin;
    float m_damage;
    float m_maxDistance;
};
