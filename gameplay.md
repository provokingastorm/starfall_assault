# Gameplay and systems guide

## Round flow

The title screen introduces the controls. Starting a run initializes a fresh pilot and begins wave one. Three waves of interceptors descend from the top of the playfield. Enemies drift sideways, reverse at the arena edges, and fire toward the pilot. When all enemies in a wave are gone, a short warning countdown starts the next wave. After wave three, the Dreadnought enters and settles near the top of the screen. Destroying it is the level victory condition.

Pause freezes the run without losing progress. Losing all hull points opens the game-over screen. Enter starts a fresh run from the title, pause, victory, or defeat screen.

## Pilot and controls

WASD or the arrow keys move the ship. Diagonal motion is normalized to the same speed as horizontal or vertical motion. Shift slows the ship and displays its small hit point, useful for navigating dense patterns. Space or Z fires paired bolts while held. The ship is constrained to the playfield.

A collision costs one hull point and grants a short invulnerability window with a flashing visual effect. Cyan energy cells restore one hull point (up to the maximum) and award a small score bonus. X uses the single emergency bomb: it clears hostile bullets, destroys active regular enemies, and deals a large amount of damage to the Dreadnought.

## Enemy patterns and boss phases

Standard enemies appear in compact, offset rows. Their slow aimed projectiles encourage the pilot to keep moving and make safe lanes through overlapping shots.

The Dreadnought has three repeating phases:

1. **Target lock:** paired shots aim at the pilot, with a short interval between volleys.
2. **Starburst:** a ring of evenly spaced projectiles expands from the boss.
3. **Crossfire:** alternating fans sweep across the arena as the core drifts between sides.

The boss health bar sits below the HUD. Hit flashes and a distinctive core make damage visible. Its patterns are designed to be read while moving; avoid chasing its horizontal drift.

## Score and feedback

Destroying enemies awards score. A destroyed enemy has a chance to drop an energy cell. Small particles, brief screen shake, contrasting projectile colors, and hit flashes give combat feedback. Sound effects use raylib's procedural waveform generation; no asset files are needed.

## Source organization

`src/main.c` contains the complete compact game:

- Configuration constants define resolution, storage capacities, timings, speeds, and balance.
- Entity structs and fixed arrays store the player, enemies, projectiles, pickups, particles, stars, and boss.
- Initialization helpers reset the run and seed the starfield.
- Update helpers move entities, spawn attacks, resolve collisions, and advance wave and boss states.
- Drawing helpers render the playfield, HUD, entities, effects, and overlays from raylib primitives.
- `main` owns the window and audio device, dispatches input and simulation, and releases resources.

## Tuning and extension

Named `const` values near the top of `src/main.c` hold movement, fire intervals, projectile speeds, spawn pacing, boss health, and phase duration. `wave_enemy_counts` controls the number of regular enemies in each wave. Fixed storage capacities are separate from encounter counts; increase a capacity if balance changes can exceed it. Add a boss attack by extending the phase branch in `update_boss` and adding its timing values with the existing settings.
