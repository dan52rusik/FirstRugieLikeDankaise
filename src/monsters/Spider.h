#pragma once

#include "Monster.h"

class Spider : public Monster {
public:
    explicit Spider(const sf::Vector2f& position);
    void updateMonster(float dt, const Player& player, const Room& room) override;

private:
    float m_dashTimer;
    float m_restTimer;
};
