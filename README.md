# Isaac Clone Baseline

Baseline project for a The Binding of Isaac-inspired roguelike built with C++17 and SFML 2.6.

Current slice:
- 960x720 SFML game window with fixed-step loop at 60 FPS
- Player movement with `WASD`
- Tear shooting with arrow keys
- Bomb placement with `E`
- Single test room with wall collisions and rocks
- Basic enemies: `Fly`, `Spider`, `Knight`
- HP, invincibility frames, bombs, minimap, simple HUD

Build:
1. Install SFML 2.6.x and CMake.
2. Ensure `SFML_DIR` or your CMake package paths are configured.
3. Run:

```powershell
cmake -S . -B build
cmake --build build
```

If `assets/fonts/isaac.ttf` is missing, the HUD still renders hearts, but text counters use no font fallback and are skipped.
