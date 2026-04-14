#pragma once
#include "Entity.h"

class Room;
class Tear;
class Bomb;
struct Item;

class Player : public Entity {
public:
    Player();

    void handleRealtimeInput();
    void update(float dt, Room& room) override;
    void draw(sf::RenderTarget& target) const override;
    void draw(sf::RenderTarget& target, float alpha) const;

    void shoot(std::vector<Tear>& tears);
    bool placeBomb(std::vector<Bomb>& bombs);
    void takeDamage(int amount);
    void takeDamage(float amount) override; // Entity override
    void heal(int amount);
    void addCoins(int amount);
    void addKeys(int amount);
    void addBombs(int amount);
    void applyItem(const Item& item);
    void setPosition(const sf::Vector2f& position) override;
    void grantInvincibility(float duration);

    sf::FloatRect getBounds() const override;
    sf::Vector2f getPosition() const override;
    int getHp() const;
    int getMaxHp() const;
    int getCoins() const;
    int getKeys() const;
    int getBombs() const;
    float getLuck() const;
    float getMoveSpeed() const;
    float getTearDamage() const;
    float getTearDelay() const;
    // isAlive() is in Entity

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
