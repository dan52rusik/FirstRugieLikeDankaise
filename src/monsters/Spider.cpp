#include "Spider.h"

#include "../Player.h"
#include "../Room.h"
#include "../utils/Collision.h"

#include "MonsterLoader.h"

Spider::Spider(const sf::Vector2f& position)
    : Monster(position, 
              MonsterLoader::get("spider").hp, 
              MonsterLoader::get("spider").speed, 
              MonsterLoader::get("spider").damage, 
              MonsterLoader::get("spider").color),
      m_dashTimer(0.0f),
      m_restTimer(0.4f) {}

void Spider::updateMonster(float dt, const Player& player, const Room&) {
    updateFlash(dt);

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
