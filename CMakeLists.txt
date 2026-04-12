cmake_minimum_required(VERSION 3.16)
project(isaac_clone LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

find_package(SFML REQUIRED COMPONENTS Graphics Window System Audio)

add_executable(isaac
    src/main.cpp
    src/Game.cpp
    src/Player.cpp
    src/Tear.cpp
    src/Bomb.cpp
    src/Room.cpp
    src/Floor.cpp
    src/Map.cpp
    src/monsters/Monster.cpp
    src/monsters/Fly.cpp
    src/monsters/Spider.cpp
    src/monsters/Knight.cpp
    src/monsters/Leech.cpp
    src/monsters/Boss.cpp
    src/items/ItemList.cpp
    src/ui/HUD.cpp
)

target_include_directories(isaac PRIVATE src)
target_link_libraries(isaac PRIVATE SFML::Graphics SFML::Window SFML::System SFML::Audio)
