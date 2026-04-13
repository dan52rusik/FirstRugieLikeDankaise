#pragma once
#include "../Entity.h"

class Player;
class Room;

class Monster : public Entity {
public:
    Monster(sf::Vector2f position, float hp, float speed, float damage, sf::Color color);
    virtual ~Monster() = default;

    // Entity interface
    void update(float dt, Room& room) override;
    virtual void updateMonster(float dt, const Player& player, const Room& room) = 0;
    
    virtual bool blocksShotFrom(const sf::Vector2f& shotOrigin) const;
    virtual bool isBoss() const;

    void draw(sf::RenderTarget& target) const override;
    void takeDamage(float damage) override;
    // kill() is now in Entity

    sf::FloatRect getBounds() const override;
    sf::Vector2f getPosition() const override;
    float getHp() const;
    float getMaxHp() const;
    float getDamage() const;
    // isAlive() is now in Entity
    void setPosition(const sf::Vector2f& position) override;

protected:
    void updateFlash(float dt);

    sf::RectangleShape m_shape;
    float m_hp;
    float m_maxHp;
    float m_speed;
    float m_damage;
    float m_flashTimer;
    sf::Color m_baseColor;
};
