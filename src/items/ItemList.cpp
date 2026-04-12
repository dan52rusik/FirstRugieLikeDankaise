#include "Item.h"

#include <vector>

std::vector<Item> createDefaultItems() {
    return {
        {"Sad Onion", "+0.7 tears"},
        {"Wire Coat Hanger", "+1.5 damage"},
        {"Skateboard", "+0.5 speed"}
    };
}
