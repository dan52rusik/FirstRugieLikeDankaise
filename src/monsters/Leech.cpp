#include "Leech.h"

#include "../Player.h"
#include "../Room.h"
#include "../utils/Collision.h"

Leech::Leech(const sf::Vector2f& position)
    : Monster(position, 20.0f, 80.0f, 2.0f, sf::Color(120, 20, 20)) {}

void Leech::update(float dt, const Player& player, const Room&) {
    const sf::Vector2f direction = Collision::normalize(Collision::subtract(player.getPosition(), m_shape.getPosition()));
    m_shape.move(Collision::scale(direction, m_speed * dt));
}
