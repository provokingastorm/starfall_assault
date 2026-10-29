# Starfall Assault

Starfall Assault is a top-down, single-level bullet hell space shooter written in C with [raylib](https://www.raylib.com/). Pilot a fast interceptor through three enemy waves, collect energy cells, and defeat the Dreadnought in the level-ending boss battle. Every visual and sound is generated at runtime; no external assets are required.

## Features

- Responsive WASD or arrow-key movement, normalized diagonals, and optional focused movement.
- Player cannons, aimed enemy shots, crossing enemy formations, and a scrolling procedural starfield.
- Three Dreadnought attack phases: aimed volleys, radial spreads, and alternating sweeping fans.
- Hull points, temporary invulnerability, score, energy pickups, and one emergency bomb.
- Start, pause, victory, and game-over screens; synthesized sound effects.
- Snake-case identifiers and descriptive, named `const` gameplay settings.

## Requirements

- C11 compiler (MSVC, Clang, or GCC), CMake 3.16+, and raylib 4.0+ installed for CMake to find.

### Windows

With vcpkg installed:

```powershell
vcpkg install raylib
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
cmake --build build --config Release
```

Run `build/Release/starfall_assault.exe` with a multi-configuration generator, or `build/starfall_assault.exe` with a single-configuration generator.

### macOS

```sh
brew install raylib cmake
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/starfall_assault
```

### Debian/Ubuntu

```sh
sudo apt install build-essential cmake libraylib-dev
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/starfall_assault
```

If a system package provides an older version, install raylib 4.0+ from its official repository, then rerun CMake.

## Controls

| Input | Action |
| --- | --- |
| W / A / S / D or arrow keys | Fly |
| Hold Space or Z | Fire |
| X | Use the one bomb |
| Hold Shift | Focused, slower movement |
| Escape or P | Pause / resume |
| Enter | Start, retry, or continue after victory |

## How to play

Survive three incoming waves to draw out the Dreadnought. Keep firing, dodge enemy projectiles, and fly into cyan energy cells to restore a hull point. The pilot starts with three hull points and one bomb. A bomb clears ordinary enemies and hostile bullets, then damages the boss. Destroy the Dreadnought to win.

## Project layout

- `src/main.c`: game loop, state, simulation, collision, drawing, and audio.
- `CMakeLists.txt`: C11 executable configuration and raylib dependency.
- `docs/gameplay.md`: round flow, attack patterns, and tuning notes.

All graphics use raylib drawing primitives. Runtime sound effects are synthesized, which keeps the source project self-contained.
