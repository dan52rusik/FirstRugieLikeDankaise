#include "Monster.h"
#include "../Room.h"
#include "../Player.h"

Monster::Monster(sf::Vector2f position, float hp, float speed, float damage, sf::Color color)
    : Entity(EntityType::Monster),
      m_hp(hp),
      m_maxHp(hp),
      m_speed(speed),
      m_damage(damage),
      m_flashTimer(0.0f),
      m_baseColor(color) {
    m_shape.setSize({36.0f, 36.0f});
    m_shape.setOrigin({18.0f, 18.0f});
    m_shape.setFillColor(color);
    m_shape.setPosition(position);
}

void Monster::update(float dt, Room& room) {
    updateMonster(dt, room.getPlayer(), room);
}

bool Monster::blocksShotFrom(const sf::Vector2f&) const {
    return false;
}

bool Monster::isBoss() const {
    return false;
}

void Monster::draw(sf::RenderTarget& target) const {
    target.draw(m_shape);
}

void Monster::takeDamage(float damage) {
    m_hp -= damage;
    if (m_hp <= 0.0f) {
        m_hp = 0.0f;
        kill();
    }
    m_flashTimer = 0.12f;
    m_shape.setFillColor(sf::Color::White);
}

void Monster::updateFlash(float dt) {
    if (m_flashTimer <= 0.0f) {
        return;
    }

    m_flashTimer -= dt;
    if (m_flashTimer <= 0.0f) {
        m_flashTimer = 0.0f;
        m_shape.setFillColor(m_baseColor);
    }
}

sf::FloatRect Monster::getBounds() const {
    return m_shape.getGlobalBounds();
}

sf::Vector2f Monster::getPosition() const {
    return m_shape.getPosition();
}

float Monster::getHp() const {
    return m_hp;
}

float Monster::getMaxHp() const {
    return m_maxHp;
}

float Monster::getDamage() const {
    return m_damage;
}

void Monster::setPosition(const sf::Vector2f& position) {
    m_shape.setPosition(position);
}
