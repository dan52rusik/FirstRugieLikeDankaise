#include "Fly.h"

#include "../Player.h"
#include "../Room.h"
#include "../utils/Collision.h"
#include "../utils/Random.h"

Fly::Fly(const sf::Vector2f& position)
    : Monster(position, 10.0f, 95.0f, 2.0f, sf::Color(70, 70, 70)),
      m_wanderDirection(1.0f, 0.0f),
      m_directionTimer(0.5f) {}

void Fly::update(float dt, const Player& player, const Room&) {
    m_directionTimer -= dt;
    if (m_directionTimer <= 0.0f) {
        m_directionTimer = 0.5f;
        const sf::Vector2f toPlayer = Collision::normalize(Collision::subtract(player.getPosition(), m_shape.getPosition()));
        const sf::Vector2f noise(Random::range(-0.6f, 0.6f), Random::range(-0.6f, 0.6f));
        m_wanderDirection = Collision::normalize(Collision::add(toPlayer, noise));
    }
    m_shape.move(Collision::scale(m_wanderDirection, m_speed * dt));
}
