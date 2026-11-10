# Quickstart: 2D & 3D Graphics Pipeline Emulation

**Branch**: `003-graphics-pipeline` | **For**: Developers implementing or reviewing the pipeline

---

## Overview

This guide explains how the new NDS graphics pipeline fits into the existing codebase and how to build, run, and test it.

---

## Architecture at a Glance

```
GX_RenderFrame()  ←── called every VBlank (59.8261 Hz)
     │
     ├── [3D path] G3_SwapBuffers() → renders into FBO → readback → gNdsGfx.g3.bg0Readback[]
     │
     ├── NdsGfx_RenderMain()
     │       └── for y = 0..191:
     │               NdsCompositor_RenderScanline(&gNdsGfx.main, y, vram, oam, pltt)
     │                    ├── NdsBg_RenderText / NdsBg_RenderAffine  (BG0–BG3)
     │                    └── NdsSprite_RenderScanline               (OAM)
     │           NdsCompositor_ApplyBrightness(main.frameBuffer, masterBright)
     │
     ├── NdsGfx_RenderSub()       (same, for sub engine)
     │
     └── NdsGfx_PresentFrame()
              └── GX_RouteCompositeBuffers() → DisplayManager_PresentFrame()
```

The test pattern (`GX_DrawTestPattern`) remains active until `GX_SetTestPatternEnabled(false)` is called. The real pipeline activates when the test pattern is disabled.

---

## Key Files

| File | Purpose |
|------|---------|
| [`port/include/nds_display_engine.h`](../../../port/include/nds_display_engine.h) | Core data types (`NdsBgLayer`, `NdsOamEntry`, `NdsDisplayEngine`) |
| [`port/hal/gx/gx_pipeline.cpp`](../../../port/hal/gx/gx_pipeline.cpp) | Pipeline lifecycle + frame entry points |
| [`port/hal/gx/nds_compositor.cpp`](../../../port/hal/gx/nds_compositor.cpp) | Per-scanline priority compositor |
| [`port/hal/gx/nds_bg_renderer.cpp`](../../../port/hal/gx/nds_bg_renderer.cpp) | BG text/affine tile rasterizer |
| [`port/hal/gx/nds_sprite_renderer.cpp`](../../../port/hal/gx/nds_sprite_renderer.cpp) | OAM sprite rasterizer |
| [`port/hal/g3/g3_pipeline.cpp`](../../../port/hal/g3/g3_pipeline.cpp) | 3D matrix stack, FBO, SwapBuffers |
| [`port/hal/g3/nds_texture_cache.cpp`](../../../port/hal/g3/nds_texture_cache.cpp) | NDS texture decoder + GL cache |

---

## Build

```bash
# Configure (from repo root)
cmake -B build -DCMAKE_BUILD_TYPE=Debug

# Build
cmake --build build -j$(nproc)
```

The graphics pipeline is compiled as part of the `port` static library target. No additional CMake flags are needed.

---

## Running the Game

```bash
./build/pokeheartgold
```

### Enabling the Real Pipeline & Authentic Game Visual Demo

By default, the native PC port launches directly into the live graphics pipeline, rendering authentic Pokémon HeartGold 2D & 3D Title Screen assets loaded from `files/demo/title/titledemo/`:
- **Top Screen (Main)**: Fixed-function 3D celestial geometry pass with spinning Sacred Gold emblem and sparkle particles rendered into the FBO and read back into BG0, blended with the authentic Pokémon HeartGold 2D Title Logo on BG1.
- **Bottom Screen (Sub/Touchscreen)**: Authentic 2D Title Screen Touch artwork on BG1, with an interactive stylus/mouse-controlled Pokéball sprite on the OBJ layer following touchscreen clicks and drags.
- **F9 Hotkey**: Press **F9** at any time while the application is running to toggle between the live authentic graphics pipeline and the interactive diagnostic test pattern card.
- **F10 Hotkey**: Cycle through dual-screen viewport layouts (Side-by-Side, Swapped Horizontal, Classic DS Vertical, Swapped Vertical).
- **F11 Hotkey**: Toggle borderless fullscreen display.

---

## Running Tests

```bash
./build/port_tests
```

Expected output for the graphics pipeline tests:
```
[PASS] BG text renderer - tile decode 4bpp
[PASS] BG text renderer - scroll wrap
[PASS] BG text renderer - affine identity
[PASS] Sprite renderer - OAM parse 128 entries
[PASS] Sprite renderer - flip H/V
[PASS] Sprite renderer - boundary clip
[PASS] Compositor - priority sort 4 layers
[PASS] Compositor - alpha blend EVA=8 EVB=8
[PASS] Compositor - master brightness +16 (white)
[PASS] Compositor - master brightness -16 (black)
[PASS] 3D pipeline - matrix stack depth tracking
[PASS] 3D pipeline - stack overflow clamp
[PASS] 3D pipeline - FBO triangle readback
```

---

## NDS-to-HAL Mapping Reference

### VRAM Pointers

```cpp
// In any renderer:
const uint8_t*  vram = gHwLcdcVram;
const uint8_t*  oam  = gHwOam;       // Main; use gHwDbOam for Sub
const uint16_t* pltt = reinterpret_cast<const uint16_t*>(gHwPltt); // Main BG palette
// HW_BG_PLTT  = offset 0   (512 bytes, 256 RGB555 colors)
// HW_OBJ_PLTT = offset 512 (512 bytes, 256 RGB555 colors)
```

### VRAM Bank Char/Screen Base Offsets

```
charBase index → VRAM byte offset = charBase * 16384
screenBase index → VRAM byte offset = screenBase * 2048
```

Both offsets are relative to `gHwLcdcVram[0]`.

### Palette Index → RGBA8

```cpp
// Use the existing helper (port/include/port_gx.h):
uint32_t rgba = GX_RGB555ToRGBA(palette[index]);
// Returns 0x00000000 conceptually for palette[0] — compositor treats color 0 as transparent.
```

### Sprite OAM Memory Layout

```
gHwOam[0..7]   → OAM entry 0
gHwOam[8..15]  → OAM entry 1
...
gHwOam[1016..1023] → OAM entry 127
Every 4th entry (entries 0,4,8,...): bytes 6–7 = affine rotation-scale parameter Pa
```

### 3D Fixed-Point Conversion

```cpp
// fx32 (Q12.20):
float f = FX32_TO_FLOAT(fx32_value);  // already defined in port_g3.h

// fx16 (Q4.12) for vertices:
float f = (float)fx16_value / (float)(1 << 12);
```

---

## Adding a New Texture Format

1. Add a decode function in `nds_texture_cache.cpp`:
   ```cpp
   static void DecodeFormat_A3I5(const uint8_t* src, const uint16_t* pltt,
                                  uint16_t w, uint16_t h, uint32_t* dst);
   ```
2. Add a `case` in `NdsTexCache_Upload`'s format dispatch switch.
3. Add a unit test in `test_g3_pipeline.cpp`.

---

## Disabling Specific Layers (Debug)

Set the `enabled` field in the BG layer struct, or clear the `visiblePlanes` bitmask in `NdsDisplayControl`:

```cpp
// Disable BG2 on Main engine:
gNdsGfx.main.dispcnt.visiblePlanes &= ~(1 << 2);  // bit 2 = BG2
```

This is the same as writing to the NDS `DISPCNT` register's visible-plane bitmask.
