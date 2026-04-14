#include "Spider.h"

#include "../Player.h"
#include "../Room.h"
#include "../utils/Collision.h"

#include "MonsterLoader.h"

Spider::Spider(const sf::Vector2f& position) {
    const auto& data = MonsterLoader::get("spider");
    m_hp = data.hp;
    m_maxHp = data.hp;
    m_speed = data.speed;
    m_damage = data.damage;
    
    m_shape.setRadius(18.0f);
    m_shape.setOrigin({18.0f, 18.0f});
    m_shape.setPosition(position);
    m_shape.setFillColor(data.color);

    m_dashTimer = 0.25f; // Начинаем с рывка
    m_restTimer = 0.4f;
}

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
