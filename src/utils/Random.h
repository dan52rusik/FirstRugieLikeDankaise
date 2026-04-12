#pragma once

#include <random>

namespace Random {
inline std::mt19937& engine() {
    static std::mt19937 rng(std::random_device{}());
    return rng;
}

inline float range(float min, float max) {
    std::uniform_real_distribution<float> dist(min, max);
    return dist(engine());
}

inline int rangeInt(int min, int max) {
    std::uniform_int_distribution<int> dist(min, max);
    return dist(engine());
}

inline bool chance(float probability) {
    return range(0.0f, 1.0f) <= probability;
}
}
