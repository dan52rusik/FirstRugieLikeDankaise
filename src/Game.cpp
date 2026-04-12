#include "Game.h"

#include <algorithm>
#include <optional>

Game::Game()
    : m_window(sf::VideoMode({960u, 720u}), "Isaac Clone"),
      m_gameOver(false) {
    m_window.setFramerateLimit(60);
    loadCurrentRoom();
}

Direction Game::opposite(Direction direction) {
    switch (direction) {
    case Direction::Up:
        return Direction::Down;
    case Direction::Down:
        return Direction::Up;
    case Direction::Left:
        return Direction::Right;
    case Direction::Right:
        return Direction::Left;
    }
    return Direction::Up;
}

void Game::loadCurrentRoom() {
    m_room.load(m_floor.getCurrentRoom());
}

void Game::tryRoomTransition() {
    if (!m_room.hasTransitionAt(m_player.getPosition())) {
        return;
    }

    const Direction direction = m_room.getDoorTransition(m_player.getPosition());
    if (!m_floor.tryMove(direction)) {
        return;
    }

    m_tears.clear();
    m_bombs.clear();
    loadCurrentRoom();
    m_player.setPosition(m_room.findSafePlayerSpawn(opposite(direction)));
    m_player.grantInvincibility(1.0f);
}

void Game::run() {
    sf::Clock clock;
    const float fixedDt = 1.0f / 60.0f;
    float accumulator = 0.0f;

    while (m_window.isOpen()) {
        const float frameTime = clock.restart().asSeconds();
        accumulator += frameTime;

        processEvents();
        while (accumulator >= fixedDt) {
            update(fixedDt);
            accumulator -= fixedDt;
        }
        render();
    }
}

void Game::processEvents() {
    while (const std::optional<sf::Event> event = m_window.pollEvent()) {
        if (event->is<sf::Event::Closed>()) {
            m_window.close();
        }

        if (!m_gameOver) {
            if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                if (keyPressed->code == sf::Keyboard::Key::E) {
                    m_player.placeBomb(m_bombs);
                }
            }
        }
    }
}

void Game::update(float dt) {
    if (m_gameOver) {
        return;
    }

    m_player.handleRealtimeInput();
    m_player.update(dt, m_room);
    m_player.shoot(m_tears);

    for (auto& tear : m_tears) {
        tear.update(dt);
    }
    for (auto& bomb : m_bombs) {
        bomb.update(dt);
    }

    m_room.update(dt, m_player, m_tears, m_bombs);
    if (m_room.isCleared()) {
        m_floor.markCurrentRoomCleared();
    }

    m_tears.erase(
        std::remove_if(m_tears.begin(), m_tears.end(), [](const Tear& tear) { return !tear.isAlive(); }),
        m_tears.end());

    m_bombs.erase(
        std::remove_if(m_bombs.begin(), m_bombs.end(), [](const Bomb& bomb) { return bomb.isFinished(); }),
        m_bombs.end());

    tryRoomTransition();
    m_gameOver = !m_player.isAlive();
}

void Game::render() {
    m_window.clear(sf::Color(25, 20, 18));
    m_room.draw(m_window);

    for (const auto& tear : m_tears) {
        tear.draw(m_window);
    }
    for (const auto& bomb : m_bombs) {
        bomb.draw(m_window);
    }

    m_player.draw(m_window);
    m_map.drawMiniMap(m_window, m_floor);
    m_hud.draw(m_window, m_player);

    if (m_gameOver) {
        sf::RectangleShape overlay({960.0f, 720.0f});
        overlay.setFillColor(sf::Color(0, 0, 0, 160));
        m_window.draw(overlay);
    }

    m_window.display();
}
