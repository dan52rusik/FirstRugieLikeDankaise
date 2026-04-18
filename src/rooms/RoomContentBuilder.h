#pragma once

class Room;
struct RoomData;

class RoomContentBuilder {
public:
    static void buildRocks(Room& room);
    static void buildProps(Room& room, RoomData& roomData);
    static void buildMonsters(Room& room, const RoomData& roomData);
    static void buildReward(Room& room, RoomData& roomData);
    static void rebuildPropInstances(Room& room);
    static void rebuildPickupInstances(Room& room);
};
