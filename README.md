# Isaac Clone Baseline

The Binding of Isaac-inspired roguelike prototype built with C++17 and SFML.

Current slice:
- 960x720 SFML game window with fixed-step loop at 60 FPS
- Player movement with `WASD`
- Tear shooting with arrow keys
- Bomb placement with `E`
- Single test room with wall collisions and rocks
- Basic enemies: `Fly`, `Spider`, `Knight`
- HP, invincibility frames, bombs, minimap, simple HUD

Build:
1. Install CMake and a C++17 compiler.
2. Run:

```powershell
cmake -S . -B build-pc
cmake --build build-pc --config Release
```

Desktop builds fetch SFML 3.0.0 automatically with `FetchContent`.
Web builds use the Emscripten-compatible SFML 2.6 branch configured in `CMakeLists.txt`.

If `assets/fonts/isaac.ttf` is missing, the HUD still renders hearts, but text counters use no font fallback and are skipped.
