#pragma once
#include <SFML/Config.hpp>
#include <SFML/Graphics.hpp>
#include <cmath>

namespace Collision {
inline sf::Vector2f scale(const sf::Vector2f& value, float factor) {
    return {value.x * factor, value.y * factor};
}

inline sf::Vector2f add(const sf::Vector2f& a, const sf::Vector2f& b) {
    return {a.x + b.x, a.y + b.y};
}

inline sf::Vector2f subtract(const sf::Vector2f& a, const sf::Vector2f& b) {
    return {a.x - b.x, a.y - b.y};
}

inline bool intersects(const sf::FloatRect& a, const sf::FloatRect& b) {
#if SFML_VERSION_MAJOR < 3
    return a.intersects(b);
#else
    return a.findIntersection(b).has_value();
#endif
}

inline float length(const sf::Vector2f& value) {
    return std::sqrt(value.x * value.x + value.y * value.y);
}

inline sf::Vector2f normalize(const sf::Vector2f& value) {
    const float len = length(value);
    if (len <= 0.0001f) {
        return {0.0f, 0.0f};
    }
    return {value.x / len, value.y / len};
}

inline float distance(const sf::Vector2f& a, const sf::Vector2f& b) {
    return length({a.x - b.x, a.y - b.y});
}
}
