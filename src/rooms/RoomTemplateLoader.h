#pragma once

#include <vector>

#include "RoomTemplate.h"

class RoomTemplateLoader {
public:
    static const std::vector<RoomTemplate>& loadAll();
    static const RoomTemplate& pick(RoomType roomType, int seed);
};
