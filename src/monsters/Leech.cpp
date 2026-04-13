#include "Leech.h"

#include "../Player.h"
#include "../Room.h"
#include "../utils/Collision.h"

#include "MonsterLoader.h"

Leech::Leech(const sf::Vector2f& position)
    : Monster(position, 
              MonsterLoader::get("leech").hp, 
              MonsterLoader::get("leech").speed, 
              MonsterLoader::get("leech").damage, 
              MonsterLoader::get("leech").color) {}

void Leech::updateMonster(float dt, const Player& player, const Room&) {
    updateFlash(dt);

    const sf::Vector2f direction = Collision::normalize(Collision::subtract(player.getPosition(), m_shape.getPosition()));
    m_shape.move(Collision::scale(direction, m_speed * dt));
}
