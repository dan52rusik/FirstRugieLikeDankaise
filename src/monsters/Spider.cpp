#include "Spider.h"

#include "../Player.h"
#include "../Room.h"
#include "../utils/Collision.h"

Spider::Spider(const sf::Vector2f& position)
    : Monster(position, 15.0f, 260.0f, 2.0f, sf::Color(55, 35, 35)),
      m_dashTimer(0.0f),
      m_restTimer(0.4f) {}

void Spider::update(float dt, const Player& player, const Room&) {
    if (m_restTimer > 0.0f) {
        m_restTimer -= dt;
        return;
    }

    m_dashTimer -= dt;
    const sf::Vector2f direction = Collision::normalize(Collision::subtract(player.getPosition(), m_shape.getPosition()));
    m_shape.move(Collision::scale(direction, m_speed * dt));

    if (m_dashTimer <= 0.0f) {
        m_dashTimer = 0.25f;
        m_restTimer = 0.8f;
    }
}
