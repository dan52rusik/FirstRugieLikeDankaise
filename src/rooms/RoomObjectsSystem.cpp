#include "rooms/RoomObjectsSystem.h"

#include <algorithm>

#include "Player.h"
#include "Room.h"
#include "utils/Random.h"

namespace {
sf::Vector2f tileCenter(int col, int row) {
    return {
        Room::kGridLeft + static_cast<float>(col) * Room::kTileSize + Room::kTileSize * 0.5f,
        Room::kGridTop + static_cast<float>(row) * Room::kTileSize + Room::kTileSize * 0.5f
    };
}

sf::Vector2f tileCenter(const sf::Vector2i& tile) {
    return tileCenter(tile.x, tile.y);
}

sf::Vector2i tileFromWorld(const sf::Vector2f& position) {
    return {
        std::clamp(static_cast<int>((position.x - Room::kGridLeft) / Room::kTileSize), 0, Room::kGridCols - 1),
        std::clamp(static_cast<int>((position.y - Room::kGridTop) / Room::kTileSize), 0, Room::kGridRows - 1)
    };
}

bool isInsideGrid(const sf::Vector2i& tile) {
    return tile.x >= 0 && tile.x < Room::kGridCols && tile.y >= 0 && tile.y < Room::kGridRows;
}
}

void RoomObjectsSystem::spawnRandomPickup(Room& room, const sf::Vector2f& position, float chance, const Player& player) {
    if (room.m_roomData == nullptr || !Random::chance(chance)) {
        return;
    }

    const int roll = Random::rangeInt(0, 99);
    PickupType type = PickupType::Coin;
    if (roll < 45) {
        type = PickupType::Coin;
    } else if (roll < 63) {
        type = PickupType::Bomb;
    } else if (roll < 81) {
        type = PickupType::Key;
    } else {
        type = (player.getHp() < player.getMaxHp()) ? PickupType::Heart : PickupType::Coin;
    }

    const sf::Vector2i dropTile = room.findFreeDropTile(position);
    room.m_roomData->pickups.push_back({type, tileCenter(dropTile), false});
    room.rebuildPickupInstances();
}

void RoomObjectsSystem::breakProp(Room& room, std::size_t propIndex, const Player& player) {
    if (room.m_roomData == nullptr || propIndex >= room.m_roomData->props.size()) {
        return;
    }

    auto& prop = room.m_roomData->props[propIndex];
    if (prop.destroyed) {
        return;
    }

    prop.destroyed = true;
    const sf::Vector2i tile = tileFromWorld(prop.position);
    if (isInsideGrid(tile)) {
        room.m_grid[room.getGridIndex(tile)] = TileContent::Empty;
    }

    room.rebuildPropInstances();
    RoomObjectsSystem::spawnRandomPickup(room, prop.position, 0.55f, player);
}

void RoomObjectsSystem::collectPickup(Room& room, std::size_t pickupIndex, Player& player) {
    if (room.m_roomData == nullptr || pickupIndex >= room.m_roomData->pickups.size()) {
        return;
    }

    auto& pickup = room.m_roomData->pickups[pickupIndex];
    if (pickup.collected) {
        return;
    }

    switch (pickup.type) {
    case PickupType::Coin:
        player.addCoins(1);
        break;
    case PickupType::Heart:
        if (player.getHp() >= player.getMaxHp()) {
            return;
        }
        player.heal(2);
        break;
    case PickupType::Key:
        player.addKeys(1);
        break;
    case PickupType::Bomb:
        player.addBombs(1);
        break;
    }

    pickup.collected = true;
    room.rebuildPickupInstances();
}

void RoomObjectsSystem::collectReward(Room& room, Player& player) {
    if (room.m_roomData == nullptr || !room.m_reward.has_value() || room.m_roomData->rewardTaken) {
        return;
    }

    player.applyItem(room.m_reward->item);
    room.m_collectedReward = room.m_reward->item;
    room.m_roomData->rewardTaken = true;
    room.m_reward.reset();
}
