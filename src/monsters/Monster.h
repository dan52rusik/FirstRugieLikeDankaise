#pragma once

#include <SFML/Graphics.hpp>

class Player;
class Room;

class Monster {
public:
    Monster(sf::Vector2f position, float hp, float speed, float damage, sf::Color color);
    virtual ~Monster() = default;

    virtual void update(float dt, const Player& player, const Room& room) = 0;
    virtual bool blocksShotFrom(const sf::Vector2f& shotOrigin) const;

    void draw(sf::RenderTarget& target) const;
    void takeDamage(float damage);
    void kill();

    sf::FloatRect getBounds() const;
    sf::Vector2f getPosition() const;
    float getDamage() const;
    bool isAlive() const;
    void setPosition(const sf::Vector2f& position);

protected:
    sf::RectangleShape m_shape;
    float m_hp;
    float m_speed;
    float m_damage;
    float m_flashTimer;
};
