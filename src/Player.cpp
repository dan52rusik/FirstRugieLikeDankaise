#include "Player.h"

#include <algorithm>
#include <cmath>

#include "Room.h"
#include "utils/Collision.h"

Player::Player()
    : m_moveInput(0.0f, 0.0f),
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
      m_maxHp(24),
      m_coins(3),
      m_keys(1),
      m_bombs(1),
      m_luck(0.0f) {
    m_body.setSize({26.0f, 30.0f});
    m_body.setOrigin({m_body.getSize().x * 0.5f, m_body.getSize().y * 0.5f});
    m_body.setFillColor(sf::Color(175, 120, 110));
    m_body.setPosition({480.0f, 360.0f});

    m_head.setRadius(18.0f);
    m_head.setOrigin({18.0f, 18.0f});
    m_head.setFillColor(sf::Color(245, 220, 210));
    m_head.setPosition({480.0f, 330.0f});
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
    m_walkTimer += dt;

    const sf::Vector2f direction = Collision::normalize(m_moveInput);
    const sf::Vector2f delta = Collision::scale(direction, m_speed * dt);

    sf::Vector2f nextBody = Collision::add(m_body.getPosition(), delta);
    sf::FloatRect nextBounds({nextBody.x - 14.0f, nextBody.y - 14.0f}, {28.0f, 28.0f});
    if (!room.collidesWithWalls(nextBounds)) {
        m_body.setPosition(nextBody);
        m_head.setPosition({nextBody.x, nextBody.y - 30.0f + std::sin(m_walkTimer * 10.0f) * 2.0f});
    }

    if (m_invincibleTimer > 0.0f && static_cast<int>(m_invincibleTimer * 12.0f) % 2 == 0) {
        m_body.setFillColor(sf::Color(255, 255, 255, 120));
        m_head.setFillColor(sf::Color(255, 255, 255, 120));
    } else {
        m_body.setFillColor(sf::Color(175, 120, 110));
        m_head.setFillColor(sf::Color(245, 220, 210));
    }
}

void Player::draw(sf::RenderTarget& target) const {
    target.draw(m_body);
    target.draw(m_head);
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

    tears.emplace_back(m_body.getPosition(), m_shotDirection, m_tearSpeed, m_tearDamage, m_shotRange);
    m_shotTimer = m_tearRate;
}

bool Player::placeBomb(std::vector<Bomb>& bombs) {
    if (m_bombs <= 0) {
        return false;
    }

    bombs.emplace_back(m_body.getPosition());
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

void Player::setPosition(const sf::Vector2f& position) {
    m_body.setPosition(position);
    m_head.setPosition({position.x, position.y - 30.0f});
}

void Player::grantInvincibility(float duration) {
    m_invincibleTimer = std::max(m_invincibleTimer, duration);
}

sf::FloatRect Player::getBounds() const {
    const sf::Vector2f position = m_body.getPosition();
    return {{position.x - 14.0f, position.y - 14.0f}, {28.0f, 28.0f}};
}

sf::Vector2f Player::getPosition() const {
    return m_body.getPosition();
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

bool Player::isAlive() const {
    return m_hp > 0;
}
