# Quickstart Guide: Game Core Integration & Window Branding

**Feature**: `002-game-core-integration`  
**Date**: 2026-11-08

This guide explains how to compile and run the native desktop recompilation of Pokémon HeartGold with the decompiled game engine (`NitroMain`) and authentic ROM window icon.

---

## 1. Building the Integrated Application

From the root of the repository:

```bash
# 1. Reconfigure CMake
cmake -B build-native -DCMAKE_BUILD_TYPE=Release

# 2. Build the executable
cmake --build build-native -j$(nproc)
```

---

## 2. Launching the Game

Run the compiled executable directly from the repository root:

```bash
./build-native/pokeheartgold
```

---

## 3. What to Expect

1. **Window Icon**:
   - The desktop window titlebar and taskbar icon will immediately display the authentic 32x32 Pokémon HeartGold cartridge icon.
2. **Game Core Boot**:
   - The game engine launches `NitroMain()`, initializes game memory heaps, RTC, sound drivers, and VFS.
   - The engine loads overlays dynamically and transitions to the intro sequence and title screen.
3. **Dual Viewports & Controls**:
   - Top Screen and Touchscreen display the rendered game graphics.
   - Press `F10` to cycle display layouts (Side-by-Side, Swapped, Vertical DS Clássico).
   - Press `F11` to toggle fullscreen.
   - Use keyboard arrow keys and `X` / `Z` / `S` / `A` for DS buttons, and mouse left-click/drag on the bottom screen for touchscreen input.
4. **Clean Exit**:
   - Close the window or press `ESC` to trigger a clean game thread shutdown and flush save progress to `pokemon_heartgold.sav`.
