#pragma once

#include <SFML/Graphics.hpp>

class Bomb {
public:
    explicit Bomb(const sf::Vector2f& position);

    void update(float dt);
    void draw(sf::RenderTarget& target) const;

    bool hasExploded() const;
    bool isFinished() const;
    bool consumeExplosion();
    float getRadius() const;
    sf::Vector2f getPosition() const;

private:
    sf::CircleShape m_shape;
    sf::CircleShape m_explosionShape;
    float m_timer;
    float m_explosionDuration;
    bool m_exploded;
    bool m_explosionConsumed;
};
