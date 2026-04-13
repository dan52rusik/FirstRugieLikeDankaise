#include "Tear.h"

#include "utils/Collision.h"

Tear::Tear(const sf::Vector2f& start, const sf::Vector2f& direction, float speed, float damage, float maxDistance)
    : Entity(EntityType::Tear),
      m_velocity(Collision::scale(direction, speed)),
      m_origin(start),
      m_damage(damage),
      m_maxDistance(maxDistance) {
    m_shape.setRadius(7.0f);
    m_shape.setOrigin({7.0f, 7.0f});
    m_shape.setFillColor(sf::Color(118, 148, 188));
    m_shape.setPosition(start);
}

void Tear::update(float dt, Room&) {
    if (!isAlive()) return;
    m_shape.move(Collision::scale(m_velocity, dt));
    if (Collision::distance(m_shape.getPosition(), m_origin) > m_maxDistance) {
        kill();
    }
}

void Tear::draw(sf::RenderTarget& target) const {
    target.draw(m_shape);
}

void Tear::destroy() {
    kill();
}

float Tear::getDamage() const {
    return m_damage;
}

sf::FloatRect Tear::getBounds() const {
    return m_shape.getGlobalBounds();
}

sf::Vector2f Tear::getPosition() const {
    return m_shape.getPosition();
}

void Tear::setPosition(const sf::Vector2f& position) {
    m_shape.setPosition(position);
}
