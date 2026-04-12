#pragma once

#include <SFML/Graphics.hpp>
#include <vector>

#include "Bomb.h"
#include "Tear.h"
#include "items/Item.h"

class Room;

class Player {
public:
    Player();

    void handleRealtimeInput();
    void update(float dt, const Room& room);
    void draw(sf::RenderTarget& target, float alpha) const;

    void shoot(std::vector<Tear>& tears);
    bool placeBomb(std::vector<Bomb>& bombs);
    void takeDamage(int amount);
    void heal(int amount);
    void addCoins(int amount);
    void addKeys(int amount);
    void addBombs(int amount);
    void applyItem(const Item& item);
    void setPosition(const sf::Vector2f& position);
    void grantInvincibility(float duration);

    sf::FloatRect getBounds() const;
    sf::Vector2f getPosition() const;
    int getHp() const;
    int getMaxHp() const;
    int getCoins() const;
    int getKeys() const;
    int getBombs() const;
    float getLuck() const;
    float getMoveSpeed() const;
    float getTearDamage() const;
    float getTearDelay() const;
    bool isAlive() const;

private:
    sf::RectangleShape m_body;
    sf::CircleShape m_head;
    sf::Vector2f m_previousPosition;
    sf::Vector2f m_position;
    sf::Vector2f m_moveInput;
    sf::Vector2f m_shotDirection;
    float m_speed;
    float m_tearDamage;
    float m_tearSpeed;
    float m_tearRate;
    float m_shotRange;
    float m_shotTimer;
    float m_invincibleTimer;
    float m_walkTimer;
    int m_hp;
    int m_maxHp;
    int m_coins;
    int m_keys;
    int m_bombs;
    float m_luck;
};
