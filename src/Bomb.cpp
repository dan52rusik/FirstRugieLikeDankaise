#include "Bomb.h"

Bomb::Bomb(const sf::Vector2f& position)
    : Entity(EntityType::Bomb),
      m_timer(2.0f),
      m_explosionDuration(0.35f),
      m_exploded(false),
      m_explosionConsumed(false) {
    m_shape.setRadius(10.0f);
    m_shape.setOrigin({10.0f, 10.0f});
    m_shape.setFillColor(sf::Color::Black);
    m_shape.setOutlineThickness(2.0f);
    m_shape.setOutlineColor(sf::Color(240, 220, 120));
    m_shape.setPosition(position);

    m_explosionShape.setRadius(100.0f);
    m_explosionShape.setOrigin({100.0f, 100.0f});
    m_explosionShape.setFillColor(sf::Color(255, 180, 80, 110));
    m_explosionShape.setOutlineThickness(4.0f);
    m_explosionShape.setOutlineColor(sf::Color(255, 210, 120));
    m_explosionShape.setPosition(position);
}

void Bomb::update(float dt, Room&) {
    if (!m_exploded) {
        m_timer -= dt;
        const bool blink = static_cast<int>(m_timer * 10.0f) % 2 == 0;
        m_shape.setFillColor(blink ? sf::Color(25, 25, 25) : sf::Color(110, 110, 110));
        if (m_timer <= 0.0f) {
            m_exploded = true;
        }
        return;
    }

    m_explosionDuration -= dt;
}

void Bomb::draw(sf::RenderTarget& target) const {
    if (!m_exploded) {
        target.draw(m_shape);
        return;
    }

    if (m_explosionDuration > 0.0f) {
        target.draw(m_explosionShape);
    }
}

bool Bomb::hasExploded() const {
    return m_exploded;
}

bool Bomb::isFinished() const {
    return m_exploded && m_explosionDuration <= 0.0f;
}

bool Bomb::consumeExplosion() {
    if (!m_exploded || m_explosionConsumed) {
        return false;
    }
    m_explosionConsumed = true;
    return true;
}

float Bomb::getRadius() const {
    return m_explosionShape.getRadius();
}

sf::Vector2f Bomb::getPosition() const {
    return m_shape.getPosition();
}

sf::FloatRect Bomb::getBounds() const {
    return m_shape.getGlobalBounds();
}

void Bomb::setPosition(const sf::Vector2f& position) {
    m_shape.setPosition(position);
    m_explosionShape.setPosition(position);
}
