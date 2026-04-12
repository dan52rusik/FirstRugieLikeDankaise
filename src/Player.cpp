#include "Player.h"

#include <algorithm>
#include <cmath>

#include "Room.h"
#include "utils/Collision.h"

Player::Player()
    : m_previousPosition(480.0f, 360.0f),
      m_position(480.0f, 360.0f),
      m_moveInput(0.0f, 0.0f),
      m_shotDirection(0.0f, -1.0f),
      m_speed(220.0f),
      m_tearDamage(3.5f),
      m_tearSpeed(480.0f),
      m_tearRate(0.3f),
      m_shotRange(400.0f),
      m_shotTimer(0.0f),
      m_invincibleTimer(0.0f),
      m_walkTimer(0.0f),
      m_hp(6),
      m_maxHp(6),
      m_coins(0),
      m_keys(1),
      m_bombs(1),
      m_luck(0.0f) {
    m_body.setSize({26.0f, 30.0f});
    m_body.setOrigin({m_body.getSize().x * 0.5f, m_body.getSize().y * 0.5f});
    m_body.setFillColor(sf::Color(175, 120, 110));
    m_body.setPosition(m_position);

    m_head.setRadius(18.0f);
    m_head.setOrigin({18.0f, 18.0f});
    m_head.setFillColor(sf::Color(245, 220, 210));
    m_head.setPosition({m_position.x, m_position.y - 30.0f});
}

void Player::handleRealtimeInput() {
    m_moveInput = {0.0f, 0.0f};
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) {
        m_moveInput.y -= 1.0f;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) {
        m_moveInput.y += 1.0f;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) {
        m_moveInput.x -= 1.0f;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) {
        m_moveInput.x += 1.0f;
    }

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up)) {
        m_shotDirection = {0.0f, -1.0f};
    } else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down)) {
        m_shotDirection = {0.0f, 1.0f};
    } else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left)) {
        m_shotDirection = {-1.0f, 0.0f};
    } else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)) {
        m_shotDirection = {1.0f, 0.0f};
    }
}

void Player::update(float dt, const Room& room) {
    m_shotTimer -= dt;
    m_invincibleTimer -= dt;
    m_previousPosition = m_position;

    const sf::Vector2f direction = Collision::normalize(m_moveInput);
    const sf::Vector2f delta = Collision::scale(direction, m_speed * dt);
    const bool moving = (delta.x != 0.0f || delta.y != 0.0f);

    if (moving) {
        m_walkTimer += dt;
    }

    // Пробуем двигаться по X
    sf::Vector2f currentPos = m_position;
    sf::Vector2f nextPosX = currentPos;
    nextPosX.x += delta.x;
    sf::FloatRect boundsX({nextPosX.x - 14.0f, nextPosX.y - 14.0f}, {28.0f, 28.0f});
    if (!room.collidesWithWalls(boundsX)) {
        m_position = nextPosX;
    }

    // Пробуем двигаться по Y (уже с учетом обновленного X)
    currentPos = m_position;
    sf::Vector2f nextPosY = currentPos;
    nextPosY.y += delta.y;
    sf::FloatRect boundsY({nextPosY.x - 14.0f, nextPosY.y - 14.0f}, {28.0f, 28.0f});
    if (!room.collidesWithWalls(boundsY)) {
        m_position = nextPosY;
    }

    // Обновляем голову всегда на основе итоговой позиции
    if (m_invincibleTimer > 0.0f && static_cast<int>(m_invincibleTimer * 12.0f) % 2 == 0) {
        m_body.setFillColor(sf::Color(255, 255, 255, 120));
        m_head.setFillColor(sf::Color(255, 255, 255, 120));
    } else {
        m_body.setFillColor(sf::Color(175, 120, 110));
        m_head.setFillColor(sf::Color(245, 220, 210));
    }
}

void Player::draw(sf::RenderTarget& target, float alpha) const {
    const sf::Vector2f renderPosition{
        m_previousPosition.x + (m_position.x - m_previousPosition.x) * alpha,
        m_previousPosition.y + (m_position.y - m_previousPosition.y) * alpha
    };
    const bool moving = std::abs(m_position.x - m_previousPosition.x) > 0.001f ||
                        std::abs(m_position.y - m_previousPosition.y) > 0.001f;
    const float bob = moving ? std::sin(m_walkTimer * 10.0f) * 2.0f : 0.0f;

    sf::RectangleShape body = m_body;
    body.setPosition(renderPosition);
    sf::CircleShape head = m_head;
    head.setPosition({renderPosition.x, renderPosition.y - 30.0f + bob});

    target.draw(body);
    target.draw(head);
}

void Player::shoot(std::vector<Tear>& tears) {
    if (m_shotTimer > 0.0f) {
        return;
    }

    if (!sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up) &&
        !sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down) &&
        !sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left) &&
        !sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)) {
        return;
    }

    tears.emplace_back(m_position, m_shotDirection, m_tearSpeed, m_tearDamage, m_shotRange);
    m_shotTimer = m_tearRate;
}

bool Player::placeBomb(std::vector<Bomb>& bombs) {
    if (m_bombs <= 0) {
        return false;
    }

    bombs.emplace_back(m_position);
    --m_bombs;
    return true;
}

void Player::takeDamage(int amount) {
    if (m_invincibleTimer > 0.0f) {
        return;
    }
    m_hp = std::max(0, m_hp - amount);
    m_invincibleTimer = 1.5f;
}

void Player::heal(int amount) {
    m_hp = std::min(m_maxHp, m_hp + amount);
}

void Player::addCoins(int amount) {
    m_coins = std::max(0, m_coins + amount);
}

void Player::addKeys(int amount) {
    m_keys = std::max(0, m_keys + amount);
}

void Player::addBombs(int amount) {
    m_bombs = std::max(0, m_bombs + amount);
}

void Player::applyItem(const Item& item) {
    switch (item.effect) {
    case ItemEffect::TearRate:
        m_tearRate = std::clamp(m_tearRate + item.amount, 0.12f, 0.75f);
        break;
    case ItemEffect::Damage:
        m_tearDamage = std::clamp(m_tearDamage + item.amount, 1.0f, 12.0f);
        break;
    case ItemEffect::Speed:
        m_speed = std::clamp(m_speed + item.amount, 140.0f, 340.0f);
        break;
    }
}

void Player::setPosition(const sf::Vector2f& position) {
    m_previousPosition = position;
    m_position = position;
    m_body.setPosition(position);
    m_head.setPosition({position.x, position.y - 30.0f});
}

void Player::grantInvincibility(float duration) {
    m_invincibleTimer = std::max(m_invincibleTimer, duration);
}

sf::FloatRect Player::getBounds() const {
    return {{m_position.x - 14.0f, m_position.y - 14.0f}, {28.0f, 28.0f}};
}

sf::Vector2f Player::getPosition() const {
    return m_position;
}

int Player::getHp() const {
    return m_hp;
}

int Player::getMaxHp() const {
    return m_maxHp;
}

int Player::getCoins() const {
    return m_coins;
}

int Player::getKeys() const {
    return m_keys;
}

int Player::getBombs() const {
    return m_bombs;
}

float Player::getLuck() const {
    return m_luck;
}

float Player::getMoveSpeed() const {
    return m_speed;
}

float Player::getTearDamage() const {
    return m_tearDamage;
}

float Player::getTearDelay() const {
    return m_tearRate;
}

bool Player::isAlive() const {
    return m_hp > 0;
}
