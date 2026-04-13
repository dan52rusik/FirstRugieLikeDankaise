#include "Fly.h"

#include "../Player.h"
#include "../Room.h"
#include "../utils/Collision.h"
#include "../utils/Random.h"

#include "MonsterLoader.h"

Fly::Fly(const sf::Vector2f& position)
    : Monster(position, 
              MonsterLoader::get("fly").hp, 
              MonsterLoader::get("fly").speed, 
              MonsterLoader::get("fly").damage, 
              MonsterLoader::get("fly").color),
      m_wanderDirection(1.0f, 0.0f),
      m_directionTimer(0.5f) {}

void Fly::updateMonster(float dt, const Player& player, const Room&) {
    updateFlash(dt);

    m_directionTimer -= dt;
    if (m_directionTimer <= 0.0f) {
        m_directionTimer = 0.5f;
        const sf::Vector2f toPlayer = Collision::normalize(Collision::subtract(player.getPosition(), m_shape.getPosition()));
        const sf::Vector2f noise(Random::range(-0.6f, 0.6f), Random::range(-0.6f, 0.6f));
        m_wanderDirection = Collision::normalize(Collision::add(toPlayer, noise));
    }
    m_shape.move(Collision::scale(m_wanderDirection, m_speed * dt));
}
