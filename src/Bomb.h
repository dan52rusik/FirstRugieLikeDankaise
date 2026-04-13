#pragma once
#include "Entity.h"

class Bomb : public Entity {
public:
    explicit Bomb(const sf::Vector2f& position);

    void update(float dt, Room& room) override;
    void draw(sf::RenderTarget& target) const override;

    bool hasExploded() const;
    bool isFinished() const;
    bool consumeExplosion();
    float getRadius() const;
    sf::FloatRect getBounds() const override;
    sf::Vector2f getPosition() const override;
    void setPosition(const sf::Vector2f& position) override;

private:
    sf::CircleShape m_shape;
    sf::CircleShape m_explosionShape;
    float m_timer;
    float m_explosionDuration;
    bool m_exploded;
    bool m_explosionConsumed;
};
