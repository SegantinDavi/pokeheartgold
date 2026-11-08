# Quickstart Guide: Pokémon HeartGold Native PC Port

This guide provides instructions for configuring, building, and running the native PC recompilation of **Pokémon HeartGold**.

---

## 1. Prerequisites

Ensure your host development environment has the necessary toolchain and libraries installed:

### Linux (Ubuntu / Debian)
```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake libsdl2-dev libgl1-mesa-dev libglu1-mesa-dev
```

### Linux (Arch / Fedora)
```bash
# Arch Linux
sudo pacman -S base-devel cmake sdl2 mesa
# Fedora
sudo dnf install @development-tools cmake SDL2-devel mesa-libGL-devel
```

### macOS
```bash
brew install cmake sdl2
```

### Windows (MSVC)
- Visual Studio 2022 with Desktop development with C++ workload.
- Install SDL2 via `vcpkg install sdl2:x64-windows` or download SDL2 development libraries.

---

## 2. Building the Native Executable

From the root of the repository:

```bash
# 1. Create a native build directory
cmake -B build-native -DCMAKE_BUILD_TYPE=Release

# 2. Compile the project
cmake --build build-native -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
```

The resulting standalone executable will be located at:
- Linux / macOS: `build-native/pokeheartgold`
- Windows: `build-native/Release/pokeheartgold.exe`

---

## 3. Running the Native Port

Run the executable directly from the repository root (so it can locate the `files/` asset directory):

```bash
./build-native/pokeheartgold
```

---

## 4. Controls & Display Configuration

### Dual-Screen Layouts
By default, the game opens in **Horizontal Side-by-Side** mode (Top Screen on the left, Touchscreen on the right).

- **Cycle Layout (F10)**: Press `F10` at runtime to cycle between:
  1. *Horizontal Side-by-Side (Default)*: Top Screen Left, Touchscreen Right
  2. *Swapped Horizontal*: Touchscreen Left, Top Screen Right
  3. *Vertical Classic DS*: Top Screen Above, Touchscreen Below
  4. *Swapped Vertical*: Touchscreen Above, Top Screen Below
- **Toggle Fullscreen (F11)**: Press `F11` to toggle between windowed mode and borderless fullscreen.

### In-Game Controls

| Action / Button | Keyboard Key | Modern Gamepad Button |
|---|---|---|
| **D-Pad Up** | Up Arrow | D-Pad Up / Left Analog Stick Up |
| **D-Pad Down** | Down Arrow | D-Pad Down / Left Analog Stick Down |
| **D-Pad Left** | Left Arrow | D-Pad Left / Left Analog Stick Left |
| **D-Pad Right** | Right Arrow | D-Pad Right / Left Analog Stick Right |
| **A Button** | `X` | Button A (South) |
| **B Button** | `Z` | Button B (East) |
| **X Button** | `S` | Button X (North) |
| **Y Button** | `A` | Button Y (West) |
| **L Trigger** | `Q` | Left Shoulder (LB / L1) |
| **R Trigger** | `W` | Right Shoulder (RB / R1) |
| **Start** | Enter / Return | Start / Options |
| **Select** | Backspace | Select / Back / Share |
| **Touchscreen** | **Mouse Left Click & Drag** | Mouse / Touchpad |

---

## 5. Save Data

Save progress is automatically written to `pokemon_heartgold.sav` in the working directory (512 KB standard format). You can backup, transfer, or restore your save file to/from real hardware or emulators seamlessly.