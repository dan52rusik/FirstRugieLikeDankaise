#include "Monster.h"

Monster::Monster(sf::Vector2f position, float hp, float speed, float damage, sf::Color color)
    : m_hp(hp),
      m_speed(speed),
      m_damage(damage),
      m_flashTimer(0.0f) {
    m_shape.setSize({36.0f, 36.0f});
    m_shape.setOrigin({18.0f, 18.0f});
    m_shape.setFillColor(color);
    m_shape.setPosition(position);
}

bool Monster::blocksShotFrom(const sf::Vector2f&) const {
    return false;
}

void Monster::draw(sf::RenderTarget& target) const {
    target.draw(m_shape);
}

void Monster::takeDamage(float damage) {
    m_hp -= damage;
    if (m_hp < 0.0f) {
        m_hp = 0.0f;
    }
}

void Monster::kill() {
    m_hp = 0.0f;
}

sf::FloatRect Monster::getBounds() const {
    return m_shape.getGlobalBounds();
}

sf::Vector2f Monster::getPosition() const {
    return m_shape.getPosition();
}

float Monster::getDamage() const {
    return m_damage;
}

bool Monster::isAlive() const {
    return m_hp > 0.0f;
}

void Monster::setPosition(const sf::Vector2f& position) {
    m_shape.setPosition(position);
}
