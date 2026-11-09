# Data Model: 2D & 3D Graphics Pipeline Emulation

**Branch**: `003-graphics-pipeline` | **Date**: 2026-11-09  
**Input**: `research.md` (Phase 0 complete)

---

## 1. Core Entities

### `NdsBgLayer` — Background Plane State
Stored as an array of 4 per engine: `NdsBgLayer gMainBG[4]`, `NdsBgLayer gSubBG[4]`.

```c
typedef struct {
    bool     enabled;           // visible bit in DISPCNT
    uint8_t  priority;          // 0 (front) – 3 (back)
    uint8_t  mode;              // NDS_BGMODE_TEXT / AFFINE / AFFINE_EXT
    uint8_t  colorMode;         // 0 = 4bpp (16-color), 1 = 8bpp (256-color)
    uint8_t  screenBase;        // 2KB block index → offset = screenBase * 2048
    uint8_t  charBase;          // 16KB block index → offset = charBase * 16384
    uint8_t  screenSize;        // 0–3: 256x256 / 512x256 / 256x512 / 512x512
    uint8_t  mosaicEnable;
    bool     extPltt;           // use extended palette bank (512 colors)
    int16_t  scrollX;           // pixel scroll offsets (wrapping)
    int16_t  scrollY;
    // Affine-only fields (BG2/BG3)
    int32_t  affineRefX;        // fx20.8 reference point
    int32_t  affineRefY;
    int16_t  affineA, affineB;  // 2x2 rotation-scale matrix (fx8.8)
    int16_t  affineC, affineD;
    bool     areaOver;          // wrap or clamp out-of-bounds
} NdsBgLayer;
```

**Validation rules**:
- `charBase` range: 0–3 (Main, text BG0/BG1); 0–15 varies by VRAM bank config.
- `screenBase` and `charBase` must not overlap in VRAM.
- BG0 in 3D mode has no char/screen data — it is fed from the 3D FBO readback.

---

### `NdsOamEntry` — Sprite Object Attribute
Packed directly from `gHwOam[1024]` / `gHwDbOam[1024]` (128 × 8 bytes each).

```c
typedef struct {
    // Attribute 0
    uint8_t  y;              // Y coordinate (wrapping at 192)
    bool     affineFlag;     // rotation/scale enabled
    bool     doubleSized;    // only if affineFlag=true
    uint8_t  objMode;        // 0=normal, 1=alpha blend, 2=OBJ window, 3=bitmap
    bool     mosaicEnable;
    uint8_t  colorMode;      // 0=4bpp, 1=8bpp
    uint8_t  shape;          // 0=square, 1=wide, 2=tall
    // Attribute 1
    int16_t  x;              // X coordinate (signed 9-bit, −256..511)
    uint8_t  affineIndex;    // index into OAM affine parameter set (if affineFlag)
    bool     flipH;          // horizontal flip (if !affineFlag)
    bool     flipV;          // vertical flip (if !affineFlag)
    uint8_t  size;           // combined with shape → pixel dimensions
    // Attribute 2
    uint16_t charName;       // character base index (4bpp: tile#; 8bpp: tile#)
    uint8_t  priority;       // 0–3
    uint8_t  paletteIndex;   // 4bpp: 0–15 palette slot; 8bpp: ignored
} NdsOamEntry;
```

**Sprite size lookup table** (`shape` × `size` → width × height):

| Shape\Size | 0 | 1 | 2 | 3 |
|------------|---|---|---|---|
| Square (0) | 8×8 | 16×16 | 32×32 | 64×64 |
| Wide (1) | 16×8 | 32×8 | 32×16 | 64×32 |
| Tall (2) | 8×16 | 8×32 | 16×32 | 32×64 |

**Validation rules**:
- OAM entries with `objMode == 3` (bitmap) ignored in this iteration (Phase 4 scope).
- Affine OAM entries share affine parameter slots (OAM bytes 6–7 of every 4th entry).

---

### `NdsPalette` — Color Lookup Table

```c
// Two banks per engine: BG palette and OBJ palette
// Each bank = 512 bytes = 256 × RGB555 entries
typedef uint16_t NdsPaletteBank[256]; // RGB555 entries

// Extended palette: 8192 bytes = 16 banks × 256 entries
typedef uint16_t NdsExtPaletteBank[16][256];

// In-memory layout (already in port_hw_registers.h):
// HW_BG_PLTT  → gHwPltt[0..511]    (Main BG  palette)
// HW_OBJ_PLTT → gHwPltt[512..1023] (Main OBJ palette)
// HW_DB_BG_PLTT / HW_DB_OBJ_PLTT   (Sub engine)
```

**Validation rules**:
- Color 0 in any palette slot is **always transparent** except when used as the screen backdrop.
- RGB555 → RGBA8 conversion: `r = (c & 0x1F) * 255 / 31`, etc. (helper `GX_RGB555ToRGBA` already in `port_gx.h`).

---

### `NdsDisplayControl` — DISPCNT Register Mirror

```c
typedef struct {
    uint8_t  bgMode;          // 0–6
    bool     bg0Is3D;         // BG0 feeds from 3D FBO
    bool     objMapChar1D;    // OBJ character map layout
    uint8_t  visiblePlanes;   // bitmask: bits 8–12 of DISPCNT (BG0-BG3, OBJ)
    uint8_t  visibleWindows;  // bitmask: bits 13–15 (WIN0, WIN1, OBJ WIN)
    uint8_t  dispMode;        // 0=off, 1=graphics, 2=VRAM-A, 3=memory display
    bool     bgExtPltt;
    bool     objExtPltt;
    bool     hBlankObjProc;
} NdsDisplayControl;
```

---

### `NdsBlendControl` — Alpha Blend & Brightness State

```c
typedef struct {
    uint8_t  blendMode;       // 0=none, 1=alpha, 2=brighten, 3=darken
    uint8_t  srcMask;         // BLDCNT first target plane bitmask
    uint8_t  dstMask;         // BLDCNT second target plane bitmask
    uint8_t  evA;             // 0–16, source weight
    uint8_t  evB;             // 0–16, destination weight
    int8_t   masterBrightness; // −16 (black) … +16 (white)
} NdsBlendControl;
```

---

### `NdsDisplayEngine` — Per-Engine Aggregate State

```c
typedef struct {
    NdsBgLayer       bg[4];
    NdsBlendControl  blend;
    NdsDisplayControl dispcnt;
    // Pixel output buffers
    uint32_t         frameBuffer[NDS_SCREEN_HEIGHT][NDS_SCREEN_WIDTH]; // RGBA8
    // 3D BG0 readback (Main engine only; NULL for Sub engine)
    uint32_t*        bg0_3d_buffer; // 256×192 RGBA8, filled by G3_SwapBuffers
} NdsDisplayEngine;
```

---

### `Nds3DPipelineState` — 3D Engine State

```c
typedef struct {
    GXMtxMode  mtxMode;
    // Matrix stacks (NDS limits: 4 Projection, 31 ModelView+Direction, 1 Texture)
    // OpenGL manages these via glPushMatrix/glPopMatrix but we track depth counts
    int        projStackDepth;   // must stay 0–3
    int        posStackDepth;    // must stay 0–30
    int        texStackDepth;    // must stay 0
    // Texture state
    uint32_t   texVramSlot;      // current texture VRAM offset
    uint32_t   texPaletteSlot;   // current texture palette VRAM offset
    uint8_t    texFormat;        // 0–7 (NDS texture format)
    uint16_t   texWidth;
    uint16_t   texHeight;
    bool       texEnabled;
    // Polygon attributes
    uint8_t    polyMode;         // 0=modulate,1=decal,2=toon,3=shadow
    uint8_t    polyAlpha;        // 0–31 (0=wire, 31=opaque)
    bool       depthTestEnable;
    bool       cullBack;
    bool       cullFront;
    // FBO for 3D→BG0 readback
    GLuint     fbo;
    GLuint     fboColorTex;
    GLuint     fboDepthRb;
} Nds3DPipelineState;
```

---

### `NdsGraphicsPipeline` — Top-Level Singleton

```c
// Owns both display engines and the 3D pipeline
typedef struct {
    NdsDisplayEngine main;    // Main engine (top screen by default)
    NdsDisplayEngine sub;     // Sub engine (touchscreen by default)
    Nds3DPipelineState g3;    // 3D pipeline
    int dispSelect;           // GX_DISP_SELECT_MAIN_SUB or SUB_MAIN
} NdsGraphicsPipeline;

extern NdsGraphicsPipeline gNdsGfx; // singleton, defined in gx_pipeline.cpp
```

---

## 2. State Transitions

```
NdsGraphicsPipeline lifecycle:
  GX_Init() / G3_Init()
       │
       ▼
  [IDLE — no frame in progress]
       │   VBlank interrupt
       ▼
  [VBLANK — game writes OAM/VRAM via GX_Load* / G2_SetBG*Offset / G3_* commands]
       │   G3_SwapBuffers() → render 3D FBO → readback into main.bg0_3d_buffer
       │
       ▼
  [COMPOSITE — NdsCompositor_RenderFrame() builds main.frameBuffer / sub.frameBuffer]
       │
       ▼
  [PRESENT — GX_RouteCompositeBuffers() + DisplayManager_PresentFrame()]
       │
       └──► back to IDLE
```

---

## 3. VRAM Bank Mapping

The 9 VRAM banks (A–I) are mapped into `gHwLcdcVram` using the same offsets as hardware:

| Bank | Size | Default Use | Offset in `gHwLcdcVram` |
|------|------|-------------|-------------------------|
| A | 128 KB | Main BG char/affine bitmap | 0x00000 |
| B | 128 KB | Main BG char | 0x20000 |
| C | 128 KB | Sub BG char | 0x40000 |
| D | 128 KB | Main BG char | 0x60000 |
| E | 64 KB | Main BG extended palette | 0x80000 |
| F | 16 KB | Main OBJ ext palette / sprite | 0x90000 |
| G | 16 KB | Main OBJ ext palette | 0x94000 |
| H | 32 KB | Sub BG extended palette | 0x98000 |
| I | 16 KB | Sub OBJ | 0xA0000 |

> [!NOTE] The game uses NitroSDK `GX_SetBankForBG()` / `GX_SetBankForOBJ()` to remap banks at runtime. The HAL must track which bank is active for each usage type and resolve the correct byte offset dynamically.

---

## 4. Interface Summary (Contracts)

See `contracts/` directory for full C header API contracts. Summary:

| Module | Key functions |
|--------|--------------|
| `gx_pipeline.cpp` | `NdsGfx_Init`, `NdsGfx_RenderMain`, `NdsGfx_RenderSub` |
| `nds_compositor.cpp` | `NdsCompositor_RenderScanline`, `NdsCompositor_ApplyBrightness` |
| `nds_bg_renderer.cpp` | `NdsBg_RenderText`, `NdsBg_RenderAffine` |
| `nds_sprite_renderer.cpp` | `NdsSprite_ParseOam`, `NdsSprite_RenderLayer` |
| `g3_pipeline.cpp` | `G3_SwapBuffers`, `G3_PushMtx`, `G3_PopMtx`, `G3_LoadMtx44`, `G3_Perspective`, `G3_TexImageParam` |
| `nds_texture_cache.cpp` | `NdsTexCache_Upload`, `NdsTexCache_Bind`, `NdsTexCache_Flush` |
