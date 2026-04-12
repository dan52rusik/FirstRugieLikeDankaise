#include "Tear.h"

#include "utils/Collision.h"

Tear::Tear(const sf::Vector2f& start, const sf::Vector2f& direction, float speed, float damage, float maxDistance)
    : m_velocity(Collision::scale(direction, speed)),
      m_origin(start),
      m_damage(damage),
      m_maxDistance(maxDistance),
      m_alive(true) {
    m_shape.setRadius(6.0f);
    m_shape.setOrigin({6.0f, 6.0f});
    m_shape.setFillColor(sf::Color(115, 190, 255));
    m_shape.setPosition(start);
}

void Tear::update(float dt) {
    m_shape.move(Collision::scale(m_velocity, dt));
    if (Collision::distance(m_origin, m_shape.getPosition()) >= m_maxDistance) {
        m_alive = false;
    }
}

void Tear::draw(sf::RenderTarget& target) const {
    target.draw(m_shape);
}

bool Tear::isAlive() const {
    return m_alive;
}

void Tear::destroy() {
    m_alive = false;
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
