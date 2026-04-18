#include "rooms/RoomCombatSystem.h"

#include <algorithm>
#include <vector>

#include "Bomb.h"
#include "Player.h"
#include "Room.h"
#include "Tear.h"
#include "rooms/RoomObjectsSystem.h"
#include "utils/Collision.h"

void RoomCombatSystem::update(Room& room, float dt, Player& player, std::vector<Tear>& tears, std::vector<Bomb>& bombs) {
    room.m_currentPlayer = &player;

    for (auto& monster : room.m_monsters) {
        if (!monster->isAlive()) {
            continue;
        }

        monster->update(dt, room);
        room.keepMonsterInPlayableArea(*monster);
        if (Collision::intersects(monster->getBounds(), player.getBounds())) {
            player.takeDamage(static_cast<int>(monster->getDamage()));
        }
    }

    for (auto& tear : tears) {
        if (!tear.isAlive()) {
            continue;
        }

        tear.update(dt, room);

        bool hitMonster = false;
        for (auto& monster : room.m_monsters) {
            if (!monster->isAlive()) {
                continue;
            }
            if (!Collision::intersects(tear.getBounds(), monster->getBounds())) {
                continue;
            }
            if (monster->blocksShotFrom(tear.getPosition())) {
                tear.destroy();
                break;
            }
            monster->takeDamage(tear.getDamage());
            tear.destroy();
            hitMonster = true;
            break;
        }

        if (hitMonster) {
            continue;
        }

        bool hitProp = false;
        for (std::size_t propIndex = 0; propIndex < room.m_props.size(); ++propIndex) {
            if (Collision::intersects(tear.getBounds(), room.m_props[propIndex].shape.getGlobalBounds())) {
                RoomObjectsSystem::breakProp(room, room.m_props[propIndex].dataIndex, player);
                tear.destroy();
                hitProp = true;
                break;
            }
        }

        if (hitProp) {
            continue;
        }

        if (room.collidesWithWalls(tear.getBounds())) {
            tear.destroy();
        }
    }

    for (auto& bomb : bombs) {
        bomb.update(dt, room);
        if (!bomb.consumeExplosion()) {
            continue;
        }

        if (Collision::distance(player.getPosition(), bomb.getPosition()) <= bomb.getRadius()) {
            player.takeDamage(2);
        }

        for (auto& monster : room.m_monsters) {
            if (!monster->isAlive()) {
                continue;
            }
            if (Collision::distance(monster->getPosition(), bomb.getPosition()) <= bomb.getRadius()) {
                monster->takeDamage(60.0f);
            }
        }

        std::vector<std::size_t> propsToBreak;
        for (const auto& prop : room.m_props) {
            if (Collision::distance(prop.shape.getPosition(), bomb.getPosition()) <= bomb.getRadius()) {
                propsToBreak.push_back(prop.dataIndex);
            }
        }
        for (std::size_t propIndex : propsToBreak) {
            RoomObjectsSystem::breakProp(room, propIndex, player);
        }
    }

    std::vector<sf::Vector2f> defeatedMonsterPositions;
    for (const auto& monster : room.m_monsters) {
        if (!monster->isAlive()) {
            defeatedMonsterPositions.push_back(monster->getPosition());
        }
    }
    for (const auto& position : defeatedMonsterPositions) {
        RoomObjectsSystem::spawnRandomPickup(room, position, 0.28f, player);
    }

    if (room.m_reward.has_value() && Collision::intersects(player.getBounds(), room.m_reward->icon.getGlobalBounds())) {
        RoomObjectsSystem::collectReward(room, player);
    }

    for (std::size_t pickupIndex = 0; pickupIndex < room.m_pickups.size(); ++pickupIndex) {
        if (Collision::intersects(player.getBounds(), room.m_pickups[pickupIndex].shape.getGlobalBounds())) {
            RoomObjectsSystem::collectPickup(room, room.m_pickups[pickupIndex].dataIndex, player);
            break;
        }
    }

    room.m_monsters.erase(
        std::remove_if(room.m_monsters.begin(), room.m_monsters.end(), [](const std::unique_ptr<Monster>& monster) {
            return !monster->isAlive();
        }),
        room.m_monsters.end());

    room.m_cleared = room.m_monsters.empty() || room.m_roomType == RoomType::Treasure || room.m_roomType == RoomType::Start;
    if (room.m_roomData != nullptr) {
        room.m_roomData->cleared = room.m_cleared;
    }

    const float targetProgress = room.m_cleared ? 1.0f : 0.0f;
    const float doorAnimSpeed = 2.8f;
    if (room.m_doorOpenProgress < targetProgress) {
        room.m_doorOpenProgress = std::min(targetProgress, room.m_doorOpenProgress + dt * doorAnimSpeed);
    } else if (room.m_doorOpenProgress > targetProgress) {
        room.m_doorOpenProgress = std::max(targetProgress, room.m_doorOpenProgress - dt * doorAnimSpeed);
    }
}
