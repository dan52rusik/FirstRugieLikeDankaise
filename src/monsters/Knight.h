#pragma once

#include "Monster.h"

class Knight : public Monster {
public:
    explicit Knight(const sf::Vector2f& position);
    void update(float dt, const Player& player, const Room& room) override;
    bool blocksShotFrom(const sf::Vector2f& shotOrigin) const override;

private:
    sf::Vector2f m_direction;
};
