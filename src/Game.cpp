#include "Game.h"
#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#endif

#include <algorithm>
#include <optional>
#include <stdexcept>
namespace {
#ifdef __EMSCRIPTEN__
unsigned int windowWidth() { 
    double w, h;
    emscripten_get_element_css_size("#canvas", &w, &h);
    return static_cast<unsigned int>(w);
}
unsigned int windowHeight() { 
    double w, h;
    emscripten_get_element_css_size("#canvas", &w, &h);
    return static_cast<unsigned int>(h);
}
#else
unsigned int windowWidth() { return 960u; }
unsigned int windowHeight() { return 720u; }
#endif

#if SFML_VERSION_MAJOR < 3
sf::VideoMode createVideoMode() {
    return sf::VideoMode(windowWidth(), windowHeight());
}
#else
sf::VideoMode createVideoMode() {
    return sf::VideoMode({windowWidth(), windowHeight()});
}
#endif

constexpr float fixedDt() {
#ifdef __EMSCRIPTEN__
    return 1.0f / 60.0f;
#else
    return 1.0f / 120.0f;
#endif
}

#ifdef __EMSCRIPTEN__
void emscriptenFrame(void* userData) {
    static_cast<Game*>(userData)->frame();
}
#endif
}

Game::Game()
    : m_window(createVideoMode(), "Isaac Clone"),
      m_gameOver(false) {
    m_window.setVerticalSyncEnabled(true);
    
    // Создаем буфер для отрисовки всей игры в фиксированном разрешении
    // Это секрет четкости в браузерах!
#if SFML_VERSION_MAJOR < 3
    if (!m_uiBuffer.create(960, 720)) {
        throw std::runtime_error("Failed to create UI render texture.");
    }
#else
    if (!m_uiBuffer.resize({960, 720})) {
        throw std::runtime_error("Failed to resize UI render texture.");
    }
#endif
    m_uiBuffer.setSmooth(false); // КЛЮЧЕВОЙ МОМЕНТ: отключаем сглаживание
    
    // Начальная настройка вида с сохранением пропорций
    sf::View view(sf::FloatRect({0, 0}, {960, 720}));
    m_window.setView(view);

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
#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop_arg(&emscriptenFrame, this, 0, true);
#else
    while (m_window.isOpen()) {
        frame();
    }
#endif
}

void Game::frame() {
    float frameDt = m_frameClock.restart().asSeconds();
    frameDt = std::min(frameDt, 0.05f);
    m_frameAccumulator += frameDt;

    processEvents();

    const float dt = fixedDt();
    while (m_frameAccumulator >= dt) {
        update(dt);
        m_frameAccumulator -= dt;
    }

    m_renderAlpha = m_frameAccumulator / dt;
    render();
}

void Game::processEvents() {
#if SFML_VERSION_MAJOR < 3
    sf::Event event;
    while (m_window.pollEvent(event)) {
        if (event.type == sf::Event::Closed) {
            m_window.close();
        }
        if (event.type == sf::Event::Resized) {
            // Масштабируем внутреннее разрешение SFML под канвас
            // Это сделает картинку четкой!
            
            float baseWidth = 960.0f;
            float baseHeight = 720.0f;
            float targetAspectRatio = baseWidth / baseHeight;
            
            float newWidth = static_cast<float>(event.size.width);
            float newHeight = static_cast<float>(event.size.height);
            float windowAspectRatio = newWidth / newHeight;
            
            sf::View gameView;
            if (windowAspectRatio > targetAspectRatio) {
                float viewWidth = baseHeight * windowAspectRatio;
                gameView.reset({(baseWidth - viewWidth) / 2.0f, 0, viewWidth, baseHeight});
            } else {
                float viewHeight = baseWidth / windowAspectRatio;
                gameView.reset({0, (baseHeight - viewHeight) / 2.0f, baseWidth, viewHeight});
            }
            m_window.setView(gameView);
            
            // Фикс для четкости: заставляем SFML осознать новый размер в пикселях
            m_window.setView(m_window.getView()); 
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
    const bool expandedMap = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Tab);

    // Рисуем всё на внутренний холст 960x720
    m_uiBuffer.clear(sf::Color(25, 20, 18));
    m_room.draw(m_uiBuffer);

    for (const auto& tear : m_tears) {
        tear.draw(m_uiBuffer);
    }
    for (const auto& bomb : m_bombs) {
        bomb.draw(m_uiBuffer);
    }

    m_player.draw(m_uiBuffer, m_renderAlpha);
    m_map.drawMiniMap(m_uiBuffer, m_floor, expandedMap);
    
    // ВАЖНО: Мы больше не рисуем HUD здесь (в буфер 960x720), 
    // потому что теперь мы рисуем его ниже напрямую в HD!
    m_uiBuffer.display();

    // Теперь выводим этот холст в настоящее окно с ЧЕТКИМ масштабированием
    m_window.clear(sf::Color::Black);
    
    sf::Sprite screenSprite(m_uiBuffer.getTexture());
    
    // Рассчитываем масштаб так, чтобы игра занимала максимум места
    float scaleX = static_cast<float>(m_window.getSize().x) / 960.0f;
    float scaleY = static_cast<float>(m_window.getSize().y) / 720.0f;
    float scale = std::min(scaleX, scaleY);

    m_window.setView(m_window.getDefaultView()); // Используем нативное разрешение окна
    
    // Масштабируем игровой экран
    screenSprite.setScale({scale, scale});
    
    // Смещение игровой области в пикселях окна
    sf::Vector2f hudOrigin(
        (m_window.getSize().x - 960.0f * scale) / 2.0f,
        (m_window.getSize().y - 720.0f * scale) / 2.0f
    );
    
    screenSprite.setPosition(hudOrigin);
    m_window.draw(screenSprite);

    // --- ОТРИСОВКА HD ИНТЕРФЕЙСА (HUD) ---
    // Теперь рисуем напрямую в окно, используя физические пиксели
    m_hud.draw(m_window, m_player, hudOrigin, scale);
    m_hud.drawStatsPanel(
        m_window,
        m_player,
        m_itemPickupNotification,
        std::min(1.0f, m_itemPickupTimer),
        expandedMap,
        hudOrigin,
        scale);
    m_hud.drawBossBar(m_window, m_room, hudOrigin, scale);
    if (m_itemPickupNotification.has_value()) {
        m_hud.drawItemPickup(m_window, *m_itemPickupNotification, std::min(1.0f, m_itemPickupTimer), hudOrigin, scale);
    }

    if (m_gameOver) {
        sf::RectangleShape overlay({960.0f * scale, 720.0f * scale});
        overlay.setPosition(hudOrigin);
        overlay.setFillColor(sf::Color(0, 0, 0, 188));
        m_window.draw(overlay);
        m_hud.drawGameOver(m_window, hudOrigin, scale);
    }

    m_window.display();
}
