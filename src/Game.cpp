#include "Game.h"

#include <algorithm>
#include <optional>

Game::Game()
    : m_window(sf::VideoMode({960u, 720u}), "Isaac Clone"),
      m_gameOver(false) {
    m_window.setFramerateLimit(60);
    loadCurrentRoom();
}

void Game::loadCurrentRoom() {
    m_room.load(m_floor.getCurrentRoom());
}

void Game::resetRun() {
    m_player = Player();
    m_floor = Floor();
    m_room = Room();
    m_tears.clear();
    m_bombs.clear();
    m_itemPickupNotification.reset();
    m_itemPickupTimer = 0.0f;
    m_gameOver = false;
    loadCurrentRoom();
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
    m_player.setPosition(m_room.findSafePlayerSpawn(oppositeDirection(direction)));
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
#if SFML_VERSION_MAJOR < 3
    sf::Event event;
    while (m_window.pollEvent(event)) {
        if (event.type == sf::Event::Closed) {
            m_window.close();
        }
        if (event.type == sf::Event::KeyPressed) {
            if (m_gameOver) {
                if (event.key.code == sf::Keyboard::R) {
                    resetRun();
                }
            } else if (event.key.code == sf::Keyboard::E) {
                m_player.placeBomb(m_bombs);
            }
        }
    }
#else
    while (const std::optional<sf::Event> event = m_window.pollEvent()) {
        if (event->is<sf::Event::Closed>()) {
            m_window.close();
        }

        if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
            if (m_gameOver) {
                if (keyPressed->code == sf::Keyboard::Key::R) {
                    resetRun();
                }
            } else if (keyPressed->code == sf::Keyboard::Key::E) {
                m_player.placeBomb(m_bombs);
            }
        }
    }
#endif
}

void Game::update(float dt) {
    if (m_gameOver) {
        return;
    }

    m_itemPickupTimer = std::max(0.0f, m_itemPickupTimer - dt);
    if (m_itemPickupTimer <= 0.0f) {
        m_itemPickupNotification.reset();
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
    if (std::optional<Item> reward = m_room.consumeCollectedReward()) {
        m_itemPickupNotification = *reward;
        m_itemPickupTimer = 3.5f;
    }
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
    m_hud.drawBossBar(m_window, m_room);
    if (m_itemPickupNotification.has_value()) {
        m_hud.drawItemPickup(m_window, *m_itemPickupNotification, std::min(1.0f, m_itemPickupTimer));
    }

    if (m_gameOver) {
        sf::RectangleShape overlay({960.0f, 720.0f});
        overlay.setFillColor(sf::Color(0, 0, 0, 188));
        m_window.draw(overlay);
        m_hud.drawGameOver(m_window);
    }

    m_window.display();
}
