# Implementation Plan: 2D & 3D Graphics Pipeline Emulation

**Branch**: `003-graphics-pipeline` | **Date**: 2026-11-09 | **Spec**: [spec.md](spec.md)  
**Input**: Feature specification from `specs/003-graphics-pipeline/spec.md`

---

## Summary

Implement a pixel-accurate NDS graphics pipeline for the Pokémon HeartGold native PC port. The plan replaces the current placeholder `GX_DrawTestPattern` with a real per-scanline 2D compositor (backgrounds BG0–BG3, OAM sprites, palette management, alpha blending, master brightness) and extends the existing OpenGL fixed-function 3D skeleton with matrix stack management, texture binding, depth testing, perspective setup, and FBO-based readback that feeds the 3D output into the compositor's BG0 plane. All new code lives exclusively under `port/hal/gx/`, `port/hal/g3/`, and `port/include/`.

---

## Technical Context

**Language/Version**: C11 (game-side shims, NitroSDK headers) + C++17 (HAL, compositor engine, texture cache)  
**Primary Dependencies**: SDL2 (window, GL context), OpenGL 3.3 Compatibility Profile (fixed-function 3D pipeline + FBO), existing `port/hal/gx/` and `port/hal/g3/` stubs  
**Storage**: VRAM emulated in `gHwLcdcVram[656KB]`, OAM in `gHwOam[1KB]`/`gHwDbOam[1KB]`, palettes in `gHwPltt[1KB]`/`gHwDbPltt[1KB]`, MMIO registers mapped to `gHardwareIoRegMemory[8KB]` via `HW_REG_BASE` (all defined in `port_hw_registers.h`)  
**Testing**: Custom test harness in `port/tests/test_runner.cpp` (unit tests) + Interactive Authentic Visual Demo (manual validation with real ROM assets before Phase 5)  
**Target Platform**: Linux x86_64 (primary), Windows 10+ and macOS 12+ (secondary)  
**Project Type**: Single project (embedded HAL module within monorepo)  
**Performance Goals**: Full 2D+3D composite frame within 16.7 ms (59.8261 Hz); 2D compositor must complete in ≤ 4 ms per frame  
**Constraints**: No GPU required for 2D path (software rasterizer); 3D path requires OpenGL 3.3 Compatibility; `sizeof(void*) == 8` on all targets; no `src/` modifications except `#ifdef PLATFORM_NATIVE` hooks  
**Scale/Scope**: 6 new source files (~2000 LOC), 5 contract headers, 10+ unit tests

---

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

| Rule | Status | Notes |
|------|--------|-------|
| All code in `port/` only | ✅ PASS | All new files under `port/hal/gx/`, `port/hal/g3/`, `port/include/` |
| No `src/` modification | ✅ PASS | Existing `#ifdef PLATFORM_NATIVE` hooks are sufficient |
| `sizeof(void*) != sizeof(uint32_t)` respected | ✅ PASS | VRAM offsets use `uint32_t`; pointers use `const uint8_t*` |
| 64-bit pointer arithmetic via `uintptr_t` | ✅ PASS | No pointer-to-int casts planned |
| English identifiers only | ✅ PASS | All identifiers, comments, and docs in English |
| Incremental testability | ✅ PASS | Each subsystem (BG, Sprite, Blend, 3D) tested independently |
| Documentation preservation | ✅ PASS | All existing comments in gx_native.cpp / g3_native.cpp preserved |

---

## Project Structure

### Documentation (this feature)

```text
specs/003-graphics-pipeline/
├── spec.md              # Feature specification
├── plan.md              # This file (speckit.plan output)
├── research.md          # Phase 0: Architecture decisions resolved
├── data-model.md        # Phase 1: Entity definitions and relationships
├── quickstart.md        # Phase 1: Developer onboarding guide
├── contracts/           # Phase 1: C header API contracts
│   ├── gx_pipeline.h
│   ├── nds_compositor.h
│   ├── nds_bg_renderer.h
│   ├── nds_sprite_renderer.h
│   ├── g3_pipeline.h
│   └── nds_texture_cache.h
└── tasks.md             # Phase 2 output (speckit.tasks — NOT created here)
```

### Source Code Layout

```text
port/
├── include/
│   ├── port_gx.h                 # EXISTING — extend with NdsDisplayControl
│   ├── port_g3.h                 # EXISTING — no changes required
│   ├── port_hw_registers.h       # EXISTING — extend HardwareIoRegisters with blend/bright fields
│   ├── nds_display_engine.h      # NEW — NdsBgLayer, NdsOamEntry, NdsDisplayEngine types
│   ├── gx_pipeline.h             # NEW — NdsGraphicsPipeline lifecycle API
│   ├── nds_compositor.h          # NEW — scanline compositor API
│   ├── nds_bg_renderer.h         # NEW — BG text/affine renderer API
│   ├── nds_sprite_renderer.h     # NEW — OAM sprite renderer API
│   ├── g3_pipeline.h             # NEW — 3D pipeline extensions API
│   └── nds_texture_cache.h       # NEW — texture decoder/cache API
│
├── hal/
│   ├── gx/
│   │   ├── gx_native.cpp         # EXISTING — retain test pattern; wire NdsGfx_PresentFrame
│   │   ├── hw_registers.cpp      # EXISTING — extend HardwareIoRegisters init
│   │   ├── gx_pipeline.cpp       # NEW — NdsGfx_Init/RenderMain/RenderSub/PresentFrame
│   │   ├── nds_compositor.cpp    # NEW — NdsCompositor_RenderScanline/ApplyBrightness
│   │   ├── nds_bg_renderer.cpp   # NEW — NdsBg_RenderText/RenderAffine/DecodeTilePixel
│   │   └── nds_sprite_renderer.cpp # NEW — NdsSprite_ParseOam/RenderScanline
│   │
│   └── g3/
│       ├── g3_native.cpp         # EXISTING — keep existing stubs; wire to Nds3DPipelineState
│       ├── g3_pipeline.cpp       # NEW — G3_PushMtx/PopMtx/LoadMtx44/Perspective/SwapBuffers
│       └── nds_texture_cache.cpp # NEW — NdsTexCache_Upload/Bind/Flush
│
└── tests/
    ├── test_bg_renderer.cpp      # NEW — tile decode, text mode, affine mode
    ├── test_sprite_renderer.cpp  # NEW — OAM parse, sprite raster, flip, clip
    ├── test_compositor.cpp       # NEW — priority sort, alpha blend, master brightness
    └── test_g3_pipeline.cpp      # NEW — matrix stack, FBO init, perspective (requires GL)
```

**Structure Decision**: Single project embedded within the monorepo HAL. All new graphics subsystems follow the established `port/hal/{subsystem}/{module}.cpp` + `port/include/{module}.h` convention used by `display/`, `audio/`, and `input/`.

---

## Implementation Phases

### Phase A: 2D Background Renderer (Priority: P1)

**Goal**: Replace `GX_DrawTestPattern` with real tilemap output for BG0–BG3.

#### A.1 — Type Definitions & Header (`nds_display_engine.h`)
Define `NdsBgLayer`, `NdsOamEntry`, `NdsDisplayControl`, `NdsBlendControl`, `NdsDisplayEngine` as specified in `data-model.md`. Extend `HardwareIoRegisters` with `blendCntMain`, `blendCntSub`, `blendAlpha`, `masterBrightMain`, `masterBrightSub`.

#### A.2 — BG Text Renderer (`nds_bg_renderer.cpp`)
- Implement `NdsBg_DecodeTilePixel`: read 4bpp/8bpp char data from VRAM, apply H/V flip.
- Implement `NdsBg_RenderText`: for each of 256 screen pixels in the scanline, compute tile coordinates from scroll offsets + screen entry, decode the pixel, look up palette → RGBA8.
- Handle screen sizes (256×256, 512×256, 256×512, 512×512) via screen entry offset arithmetic.

#### A.3 — Affine BG Renderer (`nds_bg_renderer.cpp`)
- Implement `NdsBg_RenderAffine`: apply 2×2 rotation-scale matrix + reference point per pixel, sample char data with optional `areaOver` wrap/clamp.

#### A.4 — Pipeline Wiring (`gx_pipeline.cpp` & `gx_native.cpp`)
- Implement `NdsGfx_Init`: initialize `gNdsGfx` singleton, call `G3Pipeline_Init`.
- Unify VRAM/Palette storage: update `gx_native.cpp` to write directly to `gHwLcdcVram`, `gHwPltt`, and `gHwDbPltt`, removing disconnected local arrays (`gVram`, `gMainBGPltt`, etc.).
- Redirect `HW_REG_BASE` to `gHardwareIoRegMemory` to prevent segfaults when MMIO registers are accessed.
- Implement `NdsGfx_RenderMain` / `NdsGfx_RenderSub`: synchronize active registers from `HardwareIoRegisters` / `gHardwareIoRegMemory`, call `NdsCompositor_RenderScanline` for all 192 scanlines, then `NdsCompositor_ApplyBrightness`.
- Implement `NdsGfx_PresentFrame`: call `GX_RouteCompositeBuffers` and `DisplayManager_PresentFrame`.
- Modify `gx_native.cpp::GX_RenderFrame` to call `NdsGfx_RenderMain`, `NdsGfx_RenderSub`, `NdsGfx_PresentFrame` (guard with `!GX_IsTestPatternEnabled()`). Provide runtime F9 hotkey in `main.cpp` to toggle test pattern.

#### A.5 — Compositor Scanline Pass (`nds_compositor.cpp`)
- Implement `NdsCompositor_RenderScanline`: collect per-pixel candidates from BG0–BG3 (enabled planes only), apply priority sort, resolve winner, write RGBA8.
- Implement `NdsCompositor_ApplyBrightness`: apply darken/lighten formula to all 256×192 pixels.
- Implement `NdsCompositor_AlphaBlend`: EVA/EVB two-plane blend per channel.

---

### Phase B: OAM Sprite Renderer (Priority: P2)

**Goal**: Layer animated sprites between background planes with correct transparency and flipping.

#### B.1 — OAM Parser (`nds_sprite_renderer.cpp`)
- Implement `NdsSprite_ParseOam`: decode 128 × 8-byte OAM entries into `NdsOamEntry` structs. Handle shape/size table, affine flag, double-size, 4bpp/8bpp.

#### B.2 — Sprite Scanline Renderer (`nds_sprite_renderer.cpp`)
- Implement `NdsSprite_RenderScanline`: for each enabled OAM entry, determine if the sprite overlaps the scanline, decode affected pixels from OBJ char VRAM, apply H/V flip, look up OBJ palette, write `NdsSpritePixel` with priority annotation.
- Handle OBJ character map layouts (1D: `charName * 32` bytes from OBJ VRAM; 2D: `charName * 32` within 32-column grid).
- Clip sprites at screen edges (no wrapping, FR-005 edge case).

#### B.3 — Compositor Integration
- Extend `NdsCompositor_RenderScanline` to call `NdsSprite_RenderScanline` and merge sprite pixels between BG planes according to priority: OBJ at priority P composites between BG(P−1) and BG(P).
- Implement alpha blend for OBJ mode 1 (semi-transparent sprites).

---

### Phase C: 3D Pipeline Extensions (Priority: P3)

**Goal**: Render 3D geometry correctly and composite into BG0 of the Main engine.

#### C.1 — FBO Setup (`g3_pipeline.cpp`)
- Implement `G3Pipeline_Init`: create 256×192 FBO with `GL_RGBA8` color attachment + `GL_DEPTH_COMPONENT24` renderbuffer. Store in `Nds3DPipelineState.fbo/fboColorTex/fboDepthRb`.
- Implement `G3Pipeline_Destroy`: `glDeleteFramebuffers/Textures/Renderbuffers`.

#### C.2 — Matrix Stack (`g3_pipeline.cpp`)
- Implement `G3_PushMtx` / `G3_PopMtx(count)`: call `glPushMatrix/glPopMatrix`, track depth counters, clamp on overflow/underflow.
- Implement `G3_LoadMtx44` / `G3_MultMtx44`: convert `fx32` column-major matrix → `GLfloat[16]`, call `glLoadMatrixf/glMultMatrixf`.

#### C.3 — Projection (`g3_pipeline.cpp`)
- Implement `G3_Perspective`: compute `glFrustum` from NDS fovySin/fovyCos/aspect/near/far.
- Implement `G3_Ortho`: call `glOrtho`.

#### C.4 — Polygon Attributes (`g3_pipeline.cpp`)
- Implement `G3Pipeline_PolygonAttr`: map `cullBack/cullFront` to `glCullFace/glEnable(GL_CULL_FACE)`, `depthTestEnable` to `glEnable/glDisable(GL_DEPTH_TEST)`, `polyAlpha` to `glColor4f` alpha. Expose public SDK wrapper `G3_PolygonAttr`.

#### C.5 — Texture Cache (`nds_texture_cache.cpp`)
- Implement `NdsTexCache_Upload`: decode NDS texture format → RGBA8 (all 7 formats), upload via `glTexImage2D`, cache `GLuint` keyed by `(vramOffset, paletteOffset, format)`.
- Implement `NdsTexCache_Flush`: `glDeleteTextures` for all cached IDs.
- Implement `G3Pipeline_TexImageParam` / `G3Pipeline_TexPlttBase`: store parameters, call `NdsTexCache_Upload` + `NdsTexCache_Bind` on next `G3_Begin`. Expose public SDK wrappers `G3_TexImageParam` / `G3_TexPlttBase`.

#### C.6 — Swap Buffers & Readback (`g3_pipeline.cpp`)
- Implement `G3Pipeline_SwapBuffers`: bind FBO, render pending 3D geometry (flush accumulated draw calls), `glReadPixels(GL_RGBA, GL_UNSIGNED_BYTE)` into `state->bg0Readback`, unbind FBO. Expose public SDK wrapper `G3_SwapBuffers`.
- Wire into `NdsGfx_RenderMain`: when `dispcnt.bg0Is3D`, copy `gNdsGfx.g3.bg0Readback` pixel into BG0 slot of compositor instead of tilemap fetch.

---

### Phase D: Test Suite (Automated Unit Tests)

#### D.1 — BG Renderer Tests (`test_bg_renderer.cpp`)
- Synthetic tileset (3 known tiles) + palette → render single scanline → compare RGBA byte-by-byte against reference.
- Test scroll wrap (256-wide map, scroll past 255).
- Test affine rotation (identity matrix → same as text mode).
- Test disabled BG → all transparent.

#### D.2 — Sprite Renderer Tests (`test_sprite_renderer.cpp`)
- OAM entry at x=10, y=20, 8×8 tile, no flip → verify pixels at correct coordinates.
- H/V flip → verify mirrored output.
- Clipping: sprite at x=250, 16-wide → pixels 250–255 only.
- Color 0 pixel → transparent (0x00000000).

#### D.3 — Compositor Tests (`test_compositor.cpp`)
- Two BG layers at different priorities → higher priority wins.
- OBJ at priority 1 → composites between BG0 and BG1.
- Alpha blend EVA=8, EVB=8 → verify 50/50 mix per channel.
- Master brightness +16 → all white. −16 → all black.

#### D.4 — 3D Pipeline Tests (`test_g3_pipeline.cpp`)
- Matrix stack push/pop depth tracking.
- Stack overflow clamp (push 4 times in projection mode → 4th push ignored, no crash).
- `G3Pipeline_SwapBuffers` with identity transform + solid triangle → verify non-black pixel in FBO readback center.

---

### Phase E: Interactive Authentic Visual Demo (Manual Inspection)

**Goal**: Provide a runtime visual demonstration in `pokeheartgold` rendering real Pokémon HeartGold assets (backgrounds, sprites, and 3D preview) directly from extracted ROM files in `files/`, enabling manual visual verification of 2D/3D compositing before Phase 5.

- Load authentic Title Screen / dialogue frame graphics and palettes from `files/` into `gHwLcdcVram` and `gHwPltt`.
- Populate OAM sprites with authentic characters/cursors to verify sprite rendering and transparency.
- Provide interactive toggle (hotkey F9) to switch seamlessly between the HAL test pattern card and the live Pokémon HeartGold visual demo.

---

## Complexity Tracking

> No constitution violations. All design choices stay within established patterns.

---

## Risks & Mitigations

| Risk | Likelihood | Mitigation |
|------|-----------|-----------|
| Extended palette bank remapping changes VRAM layout mid-frame | Medium | Track VRAM bank config in `HardwareIoRegisters`; flush texture cache on `GX_SetBankFor*` calls |
| OpenGL Compatibility Profile unavailable on some Linux drivers | Low | Detect at startup; fall back to 2D-only mode if no GL context (3D renders as black BG0) |
| Affine sprite (OAM rotation/scale) complexity | Medium | Defer affine OAM to a Phase E follow-up; flag affine sprites as unsupported in Phase B |
| Test isolation without a real GL context | Medium | Use SDL2 offscreen context (SDL_GL_CreateContext with hidden window) for GL-dependent tests |
| Performance of per-scanline software compositor | Low | 192 scanlines × 256 pixels = 49,152 pixels; modern CPUs handle this in < 1 ms |

---

## Success Gate (from spec.md)

| Criterion | Measurement |
|-----------|------------|
| SC-001: 100% pixel-accurate BG tilemap | Automated pixel comparison against reference captures |
| SC-002: Sprites with correct transparency | Zero border artifacts in automated render tests |
| SC-003: Brightness fade (16 levels) | No banding in gradient test; exact formula verification |
| SC-004: 3D depth occlusion, no Z-fighting | Reference triangle-overlap test with known depths |
| SC-005: ≤ 16.7 ms full composite frame | Measured with `SDL_GetPerformanceCounter` in release build |
| SC-006: 100% test pass rate | All `port/tests/` tests green |
