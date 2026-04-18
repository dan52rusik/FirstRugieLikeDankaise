#pragma once

class Room;

class RoomVisualBuilder {
public:
    static void initialize(Room& room);
    static void applyTheme(Room& room);
    static void buildVisuals(Room& room);
};
