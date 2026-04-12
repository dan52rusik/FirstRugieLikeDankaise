#include "Item.h"

#include <vector>

std::vector<Item> createDefaultItems() {
    return {
        {"Sad Onion", "Tears up", ItemEffect::TearRate, -0.06f},
        {"Wire Coat Hanger", "Damage up", ItemEffect::Damage, 1.0f},
        {"Skateboard", "Speed up", ItemEffect::Speed, 24.0f},
        {"Rusty Needle", "Tears down", ItemEffect::TearRate, 0.06f},
        {"Cracked Lens", "Damage down", ItemEffect::Damage, -0.9f},
        {"Heavy Boots", "Speed down", ItemEffect::Speed, -24.0f}
    };
}
