#pragma once

#include "Monster.h"

class Boss : public Monster {
public:
    explicit Boss(const sf::Vector2f& position);
    void update(float dt, const Player& player, const Room& room) override;
    bool isBoss() const override;
};
