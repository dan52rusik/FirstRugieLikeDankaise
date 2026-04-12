#include "Boss.h"

#include "../Player.h"
#include "../Room.h"
#include "../utils/Collision.h"

Boss::Boss(const sf::Vector2f& position)
    : Monster(position, 300.0f, 90.0f, 2.0f, sf::Color(170, 80, 80)) {
    m_shape.setSize({72.0f, 72.0f});
    m_shape.setOrigin({36.0f, 36.0f});
}

void Boss::update(float dt, const Player& player, const Room&) {
    updateFlash(dt);

    const float phaseSpeed = (m_hp <= 150.0f) ? 145.0f : 90.0f;
    const sf::Vector2f direction = Collision::normalize(Collision::subtract(player.getPosition(), m_shape.getPosition()));
    m_shape.move(Collision::scale(direction, phaseSpeed * dt));
}
