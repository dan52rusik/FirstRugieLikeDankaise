#pragma once

#include "Monster.h"

class Fly : public Monster {
public:
    explicit Fly(const sf::Vector2f& position);
    void updateMonster(float dt, const Player& player, const Room& room) override;

private:
    sf::Vector2f m_wanderDirection;
    float m_directionTimer;
};
