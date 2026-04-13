#pragma once

#include "Monster.h"

class Leech : public Monster {
public:
    explicit Leech(const sf::Vector2f& position);
    void updateMonster(float dt, const Player& player, const Room& room) override;
};
