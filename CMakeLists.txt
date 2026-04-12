cmake_minimum_required(VERSION 3.16)
project(isaac_clone LANGUAGES CXX)

include(FetchContent)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(EMSCRIPTEN)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/web)
    set(CMAKE_EXECUTABLE_SUFFIX ".html")

    FetchContent_Declare(
        SFML
        GIT_REPOSITORY https://github.com/Zombieschannel/SFML.git
        GIT_TAG SFML-2.6.x-EMCC
    )
    set(SFML_BUILD_AUDIO OFF CACHE BOOL "" FORCE)
    set(SFML_BUILD_GRAPHICS ON CACHE BOOL "" FORCE)
    set(SFML_BUILD_WINDOW ON CACHE BOOL "" FORCE)
    set(SFML_BUILD_SYSTEM ON CACHE BOOL "" FORCE)
    set(SFML_BUILD_NETWORK OFF CACHE BOOL "" FORCE)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

    FetchContent_GetProperties(SFML)
    if(NOT sfml_POPULATED)
        FetchContent_Populate(SFML)
        execute_process(
            COMMAND ${CMAKE_COMMAND}
                    -DINPUT_FILE=${sfml_SOURCE_DIR}/include/SFML/System/String.hpp
                    -P ${CMAKE_SOURCE_DIR}/cmake/PatchSfmlString.cmake
            RESULT_VARIABLE SFML_PATCH_RESULT
        )
        if(NOT SFML_PATCH_RESULT EQUAL 0)
            message(FATAL_ERROR "Failed to patch SFML String.hpp for modern Emscripten/libc++")
        endif()
        add_subdirectory(${sfml_SOURCE_DIR} ${sfml_BINARY_DIR})
    endif()
else()
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    find_package(SFML 3 REQUIRED COMPONENTS Graphics Window System Audio)
endif()

add_executable(isaac
    src/main.cpp
    src/Game.cpp
    src/Player.cpp
    src/Tear.cpp
    src/Bomb.cpp
    src/Room.cpp
    src/Floor.cpp
    src/Map.cpp
    src/rooms/RoomTemplateLoader.cpp
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

if(EMSCRIPTEN)
    target_link_libraries(isaac PRIVATE sfml-graphics sfml-window sfml-system)
else()
    target_link_libraries(isaac PRIVATE SFML::Graphics SFML::Window SFML::System SFML::Audio)
endif()

if(EMSCRIPTEN)
    # Настройки для сборки под WebGL/Emscripten, с файлом-оболочкой, где будет Yandex SDK
    set_target_properties(isaac PROPERTIES 
        OUTPUT_NAME "index"
        LINK_FLAGS "-O3 -s USE_WEBGL2=1 -s FULL_ES2=1 -s USE_FREETYPE=1 -s ASYNCIFY -s FETCH=1 -s DISABLE_EXCEPTION_CATCHING=0 -s ASYNCIFY_STACK_SIZE=65536 --preload-file ${CMAKE_SOURCE_DIR}/assets@/assets --shell-file ${CMAKE_SOURCE_DIR}/web/index.html"
    )
endif()
