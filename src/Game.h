#pragma once

#include <SFML/Graphics.hpp>
#include <optional>
#include <vector>

#include "Bomb.h"
#include "Map.h"
#include "Player.h"
#include "Room.h"
#include "ui/HUD.h"

class Game {
public:
    Game();
    void run();

private:
    void loadCurrentRoom();
    void resetRun();
    void tryRoomTransition();

    void processEvents();
    void update(float dt);
    void render();

    sf::RenderWindow m_window;
    Player m_player;
    Floor m_floor;
    Room m_room;
    Map m_map;
    HUD m_hud;
    std::vector<Tear> m_tears;
    std::vector<Bomb> m_bombs;
    std::optional<Item> m_itemPickupNotification;
    float m_itemPickupTimer{0.0f};
    bool m_gameOver;
};
