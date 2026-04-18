#include "rooms/RoomContentBuilder.h"

#include <algorithm>
#include <memory>
#include <random>
#include <vector>

#include "Room.h"
#include "items/Item.h"
#include "monsters/Boss.h"
#include "monsters/Fly.h"
#include "monsters/Knight.h"
#include "monsters/Leech.h"
#include "monsters/Spider.h"

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

bool isInsideGrid(const sf::Vector2i& tile) {
    return tile.x >= 0 && tile.x < Room::kGridCols && tile.y >= 0 && tile.y < Room::kGridRows;
}
}

void RoomContentBuilder::buildRocks(Room& room) {
    room.m_rocks.clear();
    if (room.m_template == nullptr) {
        return;
    }

    for (const auto& tile : room.m_template->rocks) {
        sf::RectangleShape rock({Room::kTileSize, Room::kTileSize});
        rock.setOrigin({24.0f, 24.0f});
        rock.setPosition(tileCenter(tile));
        rock.setFillColor(sf::Color(120, 120, 120));
        room.m_rocks.push_back(rock);

        if (isInsideGrid(tile)) {
            room.m_grid[room.getGridIndex(tile)] = TileContent::Rock;
        }
    }
}

void RoomContentBuilder::buildProps(Room& room, RoomData& roomData) {
    if (roomData.propsGenerated) {
        return;
    }

    roomData.propsGenerated = true;
    roomData.props.clear();
    roomData.pickups.clear();

    if (roomData.type != RoomType::Normal || room.m_template == nullptr) {
        return;
    }

    for (const auto& tile : room.m_template->barrels) {
        roomData.props.push_back({PropType::Barrel, tileCenter(tile), false});
        if (isInsideGrid(tile)) {
            room.m_grid[room.getGridIndex(tile)] = TileContent::Prop;
        }
    }
}

void RoomContentBuilder::buildMonsters(Room& room, const RoomData& roomData) {
    room.m_monsters.clear();

    if (roomData.cleared || roomData.type == RoomType::Start || roomData.type == RoomType::Treasure) {
        return;
    }

    if (roomData.type == RoomType::Boss) {
        room.m_monsters.emplace_back(std::make_unique<Boss>(tileCenter(7, 2)));
        return;
    }

    if (room.m_template == nullptr) {
        return;
    }

    std::mt19937 rng(static_cast<std::mt19937::result_type>(roomData.monsterSeed));
    std::uniform_int_distribution<int> typeDist(0, 3);

    const int targetCount = std::min(static_cast<int>(room.m_template->monsterSpawns.size()), 2 + (roomData.monsterSeed % 3));
    for (int i = 0; i < targetCount; ++i) {
        const sf::Vector2f spawn = tileCenter(room.m_template->monsterSpawns[static_cast<std::size_t>(i)]);
        if (room.isSpawnBlocked(spawn)) {
            continue;
        }

        switch (typeDist(rng)) {
        case 0:
            room.m_monsters.emplace_back(std::make_unique<Fly>(spawn));
            break;
        case 1:
            room.m_monsters.emplace_back(std::make_unique<Spider>(spawn));
            break;
        case 2:
            room.m_monsters.emplace_back(std::make_unique<Knight>(spawn));
            break;
        default:
            room.m_monsters.emplace_back(std::make_unique<Leech>(spawn));
            break;
        }
    }
}

void RoomContentBuilder::buildReward(Room& room, RoomData& roomData) {
    room.m_reward.reset();
    if (roomData.type != RoomType::Treasure || roomData.rewardTaken || room.m_template == nullptr || !room.m_template->rewardTile.has_value()) {
        return;
    }

    const std::vector<Item> items = createDefaultItems();
    if (items.empty()) {
        return;
    }

    if (roomData.rewardIndex < 0 || roomData.rewardIndex >= static_cast<int>(items.size())) {
        roomData.rewardIndex = roomData.rewardSeed % static_cast<int>(items.size());
    }

    const sf::Vector2f position = tileCenter(*room.m_template->rewardTile);

    Room::RewardInstance reward;
    reward.item = items[static_cast<std::size_t>(roomData.rewardIndex)];

    reward.pedestal.setSize({34.0f, 26.0f});
    reward.pedestal.setOrigin({17.0f, 13.0f});
    reward.pedestal.setPosition({position.x, position.y + 12.0f});
    reward.pedestal.setFillColor(sf::Color(108, 84, 58));
    reward.pedestal.setOutlineColor(sf::Color(168, 132, 78));
    reward.pedestal.setOutlineThickness(2.0f);

    reward.icon.setRadius(10.0f);
    reward.icon.setOrigin({10.0f, 10.0f});
    reward.icon.setPosition({position.x, position.y - 8.0f});

    switch (reward.item.effect) {
    case ItemEffect::TearRate:
        reward.icon.setFillColor(reward.item.amount < 0.0f ? sf::Color(178, 228, 255) : sf::Color(118, 148, 188));
        break;
    case ItemEffect::Damage:
        reward.icon.setFillColor(reward.item.amount > 0.0f ? sf::Color(224, 72, 78) : sf::Color(110, 52, 58));
        break;
    case ItemEffect::Speed:
        reward.icon.setFillColor(reward.item.amount > 0.0f ? sf::Color(118, 214, 128) : sf::Color(70, 118, 74));
        break;
    }

    room.m_reward = reward;
}

void RoomContentBuilder::rebuildPropInstances(Room& room) {
    room.m_props.clear();
    if (room.m_roomData == nullptr) {
        return;
    }

    for (std::size_t i = 0; i < room.m_roomData->props.size(); ++i) {
        const auto& prop = room.m_roomData->props[i];
        if (prop.destroyed || prop.type != PropType::Barrel) {
            continue;
        }

        sf::RectangleShape barrel({30.0f, 34.0f});
        barrel.setOrigin({15.0f, 17.0f});
        barrel.setPosition(prop.position);
        barrel.setFillColor(sf::Color(114, 82, 42));
        barrel.setOutlineColor(sf::Color(70, 42, 22));
        barrel.setOutlineThickness(2.0f);
        room.m_props.push_back({barrel, i});
    }
}

void RoomContentBuilder::rebuildPickupInstances(Room& room) {
    room.m_pickups.clear();
    if (room.m_roomData == nullptr) {
        return;
    }

    for (std::size_t i = 0; i < room.m_roomData->pickups.size(); ++i) {
        const auto& pickup = room.m_roomData->pickups[i];
        if (pickup.collected) {
            continue;
        }

        sf::CircleShape shape(8.0f);
        shape.setOrigin({8.0f, 8.0f});
        shape.setPosition(pickup.position);

        switch (pickup.type) {
        case PickupType::Coin:
            shape.setFillColor(sf::Color(236, 204, 82));
            break;
        case PickupType::Heart:
            shape.setFillColor(sf::Color(220, 54, 78));
            break;
        case PickupType::Key:
            shape.setFillColor(sf::Color(210, 210, 210));
            break;
        case PickupType::Bomb:
            shape.setFillColor(sf::Color(54, 54, 58));
            break;
        }

        room.m_pickups.push_back({shape, pickup.type, i});
    }
}
