#pragma once

#include <SFML/Graphics.hpp>
#include <memory>
#include <optional>
#include <vector>

#include "Bomb.h"
#include "Floor.h"
#include "Tear.h"
#include "items/Item.h"
#include "items/Pickup.h"
#include "monsters/Monster.h"
#include "props/Prop.h"
#include "rooms/RoomTemplate.h"

class Player;

enum class TileContent {
    Empty,
    Rock,
    Prop
};

class Room {
public:
    static constexpr int kGridCols = 15;
    static constexpr int kGridRows = 9;
    static constexpr float kTileSize = 40.0f;
    static constexpr float kRoomLeft = 72.0f;
    static constexpr float kRoomTop = 36.0f;
    static constexpr float kRoomWidth = 816.0f;
    static constexpr float kRoomHeight = 528.0f;
    static constexpr float kGridLeft = kRoomLeft + (kRoomWidth - kGridCols * kTileSize) * 0.5f;
    static constexpr float kGridTop = kRoomTop + (kRoomHeight - kGridRows * kTileSize) * 0.5f;
    static constexpr float kWallThickness = 24.0f;
    static constexpr float kCollisionWallPadding = 6.0f;
    static constexpr float kDoorThickness = 18.0f;
    Room();

    void load(RoomData& roomData);
    void update(float dt, Player& player, std::vector<Tear>& tears, std::vector<Bomb>& bombs);
    void draw(sf::RenderTarget& target) const;

    bool collidesWithWalls(const sf::FloatRect& bounds) const;
    bool isCleared() const;
    bool canUseDoor(Direction direction) const;
    bool hasBoss() const;
    float getBossHpRatio() const;
    std::optional<Item> consumeCollectedReward();
    sf::Vector2f getSpawnPosition(Direction fromDirection) const;
    sf::Vector2f findSafePlayerSpawn(Direction fromDirection) const;
    Direction getDoorTransition(const sf::Vector2f& playerPosition) const;
    bool hasTransitionAt(const sf::Vector2f& playerPosition) const;

    Player& getPlayer() const;

private:
    struct PropInstance {
        sf::RectangleShape shape;
        std::size_t dataIndex;
    };

    struct PickupInstance {
        sf::CircleShape shape;
        PickupType type;
        std::size_t dataIndex;
    };

    struct RewardInstance {
        sf::RectangleShape pedestal;
        sf::CircleShape icon;
        Item item;
    };

    void buildRocks();
    void buildProps(RoomData& roomData);
    void buildMonsters(const RoomData& roomData);
    void buildReward(RoomData& roomData);
    void rebuildPropInstances();
    void rebuildPickupInstances();
    void breakProp(std::size_t propIndex, const Player& player);
    void collectPickup(std::size_t pickupIndex, Player& player);
    void collectReward(Player& player);
    void spawnRandomPickup(const sf::Vector2f& position, float chance, const Player& player);
    bool isTileOccupied(const sf::Vector2i& tile) const;
    sf::Vector2i findFreeDropTile(const sf::Vector2f& position) const;
    bool isSpawnBlocked(const sf::Vector2f& position) const;
    bool isInDoorOpening(const sf::FloatRect& bounds) const;
    void keepMonsterInPlayableArea(Monster& monster) const;
    void drawDoor(sf::RenderTarget& target, Direction direction) const;
    sf::FloatRect getDoorOpening(Direction direction) const;
    sf::FloatRect getDoorTrigger(Direction direction) const;
    int getGridIndex(const sf::Vector2i& tile) const;
    int getGridIndex(const sf::Vector2f& position) const;

    mutable sf::RectangleShape m_floor;
    sf::RectangleShape m_innerBounds;
    std::optional<sf::Sprite> m_floorSprite;
    std::optional<sf::Sprite> m_backdropSprite;
    std::optional<sf::Sprite> m_cornerSprites[4];
    std::optional<sf::Sprite> m_wallSprites[4];
    
    static sf::Texture s_floorTexture;
    static bool s_floorTextureLoaded;
    static sf::Texture s_backdropTexture;
    static bool s_backdropTextureLoaded;
    static sf::Texture s_wallTexture;
    static bool s_wallTextureLoaded;
    static sf::Texture s_doorTexture;
    static bool s_doorTextureLoaded;

    RoomData* m_roomData{nullptr};
    const RoomTemplate* m_template{nullptr};
    TileContent m_grid[kGridCols * kGridRows];
    std::vector<sf::RectangleShape> m_rocks;
    std::vector<PropInstance> m_props;
    std::vector<PickupInstance> m_pickups;
    std::optional<RewardInstance> m_reward;
    std::optional<Item> m_collectedReward;
    std::vector<std::unique_ptr<Monster>> m_monsters;
    bool m_doors[4]{false, false, false, false};
    RoomType m_roomType{RoomType::Normal};
    bool m_cleared{false};
    float m_doorOpenProgress{0.0f};
    Player* m_currentPlayer{nullptr};
};
