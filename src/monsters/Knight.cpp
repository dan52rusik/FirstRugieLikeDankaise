#include "Knight.h"

#include "../Player.h"
#include "../Room.h"
#include "../utils/Collision.h"

#include "MonsterLoader.h"

Knight::Knight(const sf::Vector2f& position)
    : Monster(position, 
              MonsterLoader::get("knight").hp, 
              MonsterLoader::get("knight").speed, 
              MonsterLoader::get("knight").damage, 
              MonsterLoader::get("knight").color) 
{
    m_direction = {1.0f, 0.0f};
}

void Knight::updateMonster(float dt, const Player&, const Room& room) {
    updateFlash(dt);

    const sf::Vector2f next = Collision::add(m_shape.getPosition(), Collision::scale(m_direction, m_speed * dt));
    const sf::FloatRect nextBounds({next.x - 18.0f, next.y - 18.0f}, {36.0f, 36.0f});
    if (room.collidesWithWalls(nextBounds)) {
        if (m_direction.x != 0.0f) {
            m_direction.x *= -1.0f;
        } else {
            m_direction.y *= -1.0f;
        }
    } else {
        m_shape.setPosition(next);
    }
}

bool Knight::blocksShotFrom(const sf::Vector2f& shotOrigin) const {
    // Рыцарь блокирует урон только спереди (в данном случае - если игрок слева)
    // Это имитирует фронтальный щит.
    return shotOrigin.x < m_shape.getPosition().x;
}
