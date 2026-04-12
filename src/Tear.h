#pragma once

#include <SFML/Graphics.hpp>

class Tear {
public:
    Tear(const sf::Vector2f& start, const sf::Vector2f& direction, float speed, float damage, float maxDistance);

    void update(float dt);
    void draw(sf::RenderTarget& target) const;

    bool isAlive() const;
    void destroy();
    float getDamage() const;
    sf::FloatRect getBounds() const;
    sf::Vector2f getPosition() const;

private:
    sf::CircleShape m_shape;
    sf::Vector2f m_velocity;
    sf::Vector2f m_origin;
    float m_damage;
    float m_maxDistance;
    bool m_alive;
};
