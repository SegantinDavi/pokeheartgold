# Research: 2D & 3D Graphics Pipeline Emulation

**Branch**: `003-graphics-pipeline` | **Date**: 2026-11-09  
**Phase**: 0 — Pre-Design Research

---

## 1. NDS 2D Engine Architecture Findings

### Decision: Software-rasterized tile renderer in a CPU-side scanline compositor
- **Rationale**: The NDS 2D engine outputs a fixed 256×192 RGBA framebuffer per VBlank. A fully-software tile blitter avoids GPU synchronization overhead for small-footprint 2D work, and allows bit-exact palette/flip/priority semantics. The result is uploaded as a GL_TEXTURE_2D and blitted to the OpenGL viewport by the existing `DisplayManager`.
- **Alternatives considered**:
  - *OpenGL fragment shader per BG layer*: Flexible but introduces shader compilation latency and mismatches DS fixed-function alpha with GLSL blending.
  - *SDL_RenderCopy tilemaps*: SDL2 renderer is incompatible with the existing SDL2+OpenGL hybrid context.

### NDS VRAM Layout (resolved)
The hardware provides the following regions, **already emulated** in `port_hw_registers.h`:

| Symbol | Address offset | Size | Purpose |
|--------|---------------|------|---------|
| `gHwLcdcVram` | 0 | 656 KB | All VRAM banks A-I (the 9 configurable banks) |
| `gHwOam` | — | 1 KB | Main OAM (128 × 8-byte entries) |
| `gHwDbOam` | — | 1 KB | Sub OAM |
| `gHwPltt` | — | 1 KB | Main palette (512 B BG + 512 B OBJ) |
| `gHwDbPltt` | — | 1 KB | Sub palette |

Convenience macros `HW_BG_PLTT`, `HW_OBJ_PLTT`, `HW_DB_BG_PLTT`, `HW_DB_OBJ_PLTT` already resolve to the correct byte offsets.

### BG Text-Mode Tile Decoding (resolved)
Screen entries are 16-bit `GXScrFmtText` with:
- Bits [9:0] → character name (tile index into char base)
- Bit 10 → HFlip; Bit 11 → VFlip
- Bits [15:12] → palette number (4bpp mode only)

Char data: 4bpp = 32 bytes/tile, 8bpp = 64 bytes/tile.

### OAM Entry Format (resolved)
Each OAM entry is 8 bytes (3 × u16 attributes + 1 × u16 affine param):
- **Attr0**: Y coordinate (8b), rotation/scale flag, double-size, obj mode (normal/affine/window/bitmap), mosaic, 4bpp/8bpp, shape (square/wide/tall)
- **Attr1**: X coordinate (9b), flip H/V, size
- **Attr2**: Character name, priority, palette

Shape + Size lookup table produces sprite dimensions (8×8 → 64×64).

### Priority Compositing Algorithm (resolved)
Per-pixel priority ordering on hardware:
1. For each screen column of a scanline, collect from all enabled planes.
2. Priority is a 2-bit field (0=front, 3=back).
3. OBJs at priority P composite between BG(P-1) and BG(P).
4. Transparent pixels (palette index 0 or OAM Mode=Window) fall through.
5. Backdrop color = BG palette entry 0 (never transparent).

**Decision**: Implement a per-scanline priority sort across all 4 BG planes + OAM batch, producing the final 256-pixel row. This matches hardware exactly for scanline-based games.

---

## 2. Alpha Blending & Master Brightness (resolved)

### Alpha Blending (`G2x_SetBlendAlpha_`)
- Register `BLDCNT` selects source/destination plane masks.
- Register `BLDALPHA` holds EVA (5 bits, source weight) and EVB (5 bits, destination weight), range 0–16.
- Formula: `out = clamp(A*EVA/16 + B*EVB/16, 0, 31)` per channel.
- **Decision**: Track `BLDCNT` and `BLDALPHA` as fields in `HardwareIoRegisters`; apply blend in compositor after priority sort.

### Master Brightness (`G2x_SetBlendBrightness_`)
- Register `MASTER_BRIGHT` bits [4:0] = level (0–16), bit 14 = direction (0=darken, 1=lighten).
- Applies to entire screen output post-composition.
- Darken: `out = color * (16 - level) / 16`.
- Lighten: `out = color + (63 - color) * level / 16` (DS uses 6-bit channels internally).
- **Decision**: Apply as a final pass on the completed 256×192 RGBA framebuffer before upload.

---

## 3. 3D Pipeline Strategy (resolved)

### Decision: OpenGL Compatibility Profile fixed-function pipeline (existing skeleton, extend)
- **Rationale**: `g3_native.cpp` already maps `G3_MtxMode`, `G3_Begin/End`, `G3_Vtx`, `G3_Color`, `G3_TexCoord`, `G3_Normal` directly to OpenGL fixed-function equivalents via `glMatrixMode/glBegin/glEnd/glVertex3f`. This is sufficient for NDS geometry semantics.
- **Missing pieces to implement**:
  - Matrix Push/Pop (`G3_PushMtx`, `G3_PopMtx`) → `glPushMatrix/glPopMatrix`
  - Perspective frustum (`G3X_SetFog`, `G3_Perspective`, `G3_Frustum`) → `glFrustum/glFog*`
  - Texture loading + binding → `glGenTextures/glTexImage2D` from VRAM texture slots
  - Depth test state (`G3X_SetFog`, polygon attributes) → `glDepthFunc/glEnable(GL_DEPTH_TEST)`
  - Render-to-texture for 3D→BG0 compositing → off-screen FBO or pixel-readback

### Decision: 3D output composited into BG0 layer via FBO readback
- **Rationale**: NDS 3D engine renders into the Main engine's BG0 slot as a 256×192 raster. We render 3D geometry into an FBO at the same resolution, then readback as RGBA into the 2D compositor's BG0 plane buffer. This preserves correct layering with 2D planes (BG1-BG3, OBJ) in the software compositor.
- **Alternatives considered**:
  - *Direct OpenGL blend with 2D quads*: Cannot replicate per-pixel priority with 2D sprites correctly.
  - *Software rasterizer for 3D*: Too slow for the full polygon budget.

### NDS Fixed-Point Precision (resolved)
- `fx32` = Q12.20 fixed-point (shift = 12). Macro `FX32_TO_FLOAT` already defined in `port_g3.h`.
- `fx16` = Q4.12 fixed-point (vertices). Conversion: `(float)v / (1 << 12)`.
- Matrix elements are `fx32` for position/projection, `fx16` for normals.

---

## 4. Texture Management (resolved)

### NDS Texture Formats
| ID | Name | BPP | Notes |
|----|------|-----|-------|
| 1 | A3I5 | 8 | 3-bit alpha, 5-bit palette index |
| 2 | 4-color | 2 | palette |
| 3 | 16-color | 4 | palette |
| 4 | 256-color | 8 | palette |
| 5 | Compressed | 4 | 4×4 block compression |
| 6 | A5I3 | 8 | 5-bit alpha, 3-bit index |
| 7 | Direct | 16 | RGB555+alpha |

**Decision**: Convert all formats to RGBA8 at texture-load time, stored in a `std::unordered_map<uint32_t, GLuint>` keyed by VRAM slot ID. Lazy conversion on first `G3_TexImageParam` call.

---

## 5. Frame Pipeline Integration (resolved)

```
VBlank interrupt (59.8261 Hz)
  │
  ├─► 3D Geometry commands accumulated in display-list buffer
  │      └─► G3_SwapBuffers() → Render 3D FBO → readback → gBg0_3D_buffer[]
  │
  ├─► 2D compositor (per-scanline):
  │      for each scanline y:
  │        collect pixels from BG0(3D or tile) / BG1 / BG2 / BG3 / OAM sprites
  │        priority-sort → alpha-blend → master-brightness → gMainPixels[y][256]
  │
  └─► GX_RenderFrame():
         GX_RouteCompositeBuffers(gMainPixels, gSubPixels)
         DisplayManager_PresentFrame(top, touch)
```

---

## 6. Test Strategy (resolved)

| Layer | Test approach |
|-------|--------------|
| BG tile rendering | Load known tileset+palette, render single BG layer, compare output RGBA against reference PNG |
| OAM sprites | Programmatic OAM entries with known positions, compare output |
| Alpha blend | Known EVA/EVB pair, verify per-channel formula |
| Master brightness | Verify darken/lighten at levels 0, 8, 16 |
| 3D geometry | Submit unit triangle, verify rasterized FBO output |
| Priority ordering | 4 BG planes + OBJ at all priority permutations |

All tests run in `port/tests/` using the existing `test_runner.cpp` harness (no GL context required for 2D; GL context mocked/headless for 3D).
