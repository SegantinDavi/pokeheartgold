# Tasks: 2D & 3D Graphics Pipeline Emulation

**Branch**: `003-graphics-pipeline`  
**Input**: Design documents from `specs/003-graphics-pipeline/`  
**Prerequisites**: plan.md ✅ · spec.md ✅ · research.md ✅ · data-model.md ✅ · contracts/ ✅ · quickstart.md ✅

**Organization**: Tasks are grouped by user story to enable independent implementation and testing of each story.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies on incomplete tasks)
- **[Story]**: Which user story this task belongs to ([US1], [US2], [US3])
- Exact file paths included in every description

## Path Conventions

All new source files live under `port/` (HAL) or `port/tests/` (tests). No `src/` modifications.

---

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: Create the new headers and extend existing registers — the shared foundation all phases depend on.

- [X] T001 Create `port/include/nds_display_engine.h` defining `NdsBgLayer`, `NdsOamEntry`, `NdsDisplayControl`, `NdsBlendControl`, `NdsDisplayEngine` structs exactly as specified in `data-model.md`
- [X] T002 Extend `port/include/port_hw_registers.h` — map `HW_REG_BASE` and `HW_IOREG` to `((uintptr_t)gHardwareIoRegMemory)` and `HW_BG_VRAM` to `((uintptr_t)gHwLcdcVram)` under `PLATFORM_NATIVE` to prevent memory segfaults; add `blendCntMain`, `blendCntSub`, `blendAlpha`, `masterBrightMain`, `masterBrightSub` fields to `HardwareIoRegisters`; update `PortHwRegisters_Init` in `port/hal/gx/hw_registers.cpp` to zero-initialize them
- [X] T003 [P] Create `port/include/gx_pipeline.h` declaring `NdsGraphicsPipeline`, `gNdsGfx` extern, and `NdsGfx_Init/RenderMain/RenderSub/PresentFrame` — exact signature per `contracts/gx_pipeline.h`
- [X] T004 [P] Create `port/include/nds_compositor.h` declaring `NdsCompositor_RenderScanline`, `NdsCompositor_ApplyBrightness`, `NdsCompositor_AlphaBlend` — exact signature per `contracts/nds_compositor.h`
- [X] T005 [P] Create `port/include/nds_bg_renderer.h` declaring `NdsBg_RenderText`, `NdsBg_RenderAffine`, `NdsBg_DecodeTilePixel` — exact signature per `contracts/nds_bg_renderer.h`
- [X] T006 [P] Create `port/include/nds_sprite_renderer.h` declaring `NdsSpritePixel`, `NdsSprite_ParseOam`, `NdsSprite_RenderScanline` — exact signature per `contracts/nds_sprite_renderer.h`
- [X] T007 [P] Create `port/include/g3_pipeline.h` declaring `Nds3DPipelineState`, `G3Pipeline_*` internal functions, and NitroSDK public forwarder shims — exact signature per `contracts/g3_pipeline.h`
- [X] T008 [P] Create `port/include/nds_texture_cache.h` declaring `NdsTexCache_Upload`, `NdsTexCache_Bind`, `NdsTexCache_Flush` — exact signature per `contracts/nds_texture_cache.h`

**Checkpoint**: All headers exist and compile cleanly (`cmake --build build -- port_headers` or equivalent). No implementation yet.

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: Core pipeline skeleton and palette conversion — blocks all user stories.

**⚠️ CRITICAL**: No user story work can begin until this phase is complete.

- [X] T009 Implement `NdsGfx_Init` in `port/hal/gx/gx_pipeline.cpp`: define `gNdsGfx` singleton, zero-initialize both `NdsDisplayEngine` fields and `Nds3DPipelineState`, call `PortHwRegisters_Init`; consolidate `port/hal/gx/gx_native.cpp` so that `GX_Load*` functions directly populate `gHwLcdcVram`, `gHwPltt`, and `gHwDbPltt` (removing local `gVram`/`gMainBGPltt` arrays); update `port/hal/gx/gx_native.cpp::GX_Init` to call `NdsGfx_Init`
- [X] T010 Implement `NdsGfx_PresentFrame` in `port/hal/gx/gx_pipeline.cpp`: call `GX_RouteCompositeBuffers` with `main.frameBuffer`/`sub.frameBuffer` respecting `gNdsGfx.dispSelect`, then `DisplayManager_PresentFrame`
- [X] T011 Wire `port/hal/gx/gx_native.cpp::GX_RenderFrame` to dispatch: when `!GX_IsTestPatternEnabled()`, call `NdsGfx_RenderMain`, `NdsGfx_RenderSub`, `NdsGfx_PresentFrame` instead of the test pattern path; bind F9 hotkey in `port/main.cpp` to toggle `GX_SetTestPatternEnabled(!GX_IsTestPatternEnabled())` dynamically at runtime
- [X] T012 [P] Implement `NdsCompositor_AlphaBlend` in `port/hal/gx/nds_compositor.cpp`: two-plane EVA/EVB blend per channel using NDS formula `out = clamp(A*EVA/16 + B*EVB/16, 0, 255)` per channel
- [X] T013 [P] Implement `NdsCompositor_ApplyBrightness` in `port/hal/gx/nds_compositor.cpp`: darken formula `out = color * (16 - level) / 16`; lighten formula `out = color + (255 - color) * level / 16`; apply to all 192×256 RGBA8 pixels; handle `level == 0` as no-op

**Checkpoint**: Build succeeds; `GX_RenderFrame` calls `NdsGfx_PresentFrame`; with test pattern disabled the screens show solid black (empty framebuffers). `NdsCompositor_AlphaBlend` and `ApplyBrightness` are callable.

---

## Phase 3: User Story 1 — 2D Background Tilemap Rendering (Priority: P1) 🎯 MVP

**Goal**: Render BG0–BG3 text-mode and affine-mode tilemaps from VRAM into both screen framebuffers with correct palette, scroll, and priority.

**Independent Test**: Load a known tileset + palette into `gHwLcdcVram`/`gHwPltt`, configure one BG layer, call `NdsGfx_RenderMain`, compare `gNdsGfx.main.frameBuffer` byte-by-byte against a reference RGBA array.

### Implementation for User Story 1

- [X] T014 [P] [US1] Implement `NdsBg_DecodeTilePixel` in `port/hal/gx/nds_bg_renderer.cpp`: read 4bpp (nibble) or 8bpp (byte) from char data with optional H/V flip; return palette index (0 = transparent)
- [X] T015 [US1] Implement `NdsBg_RenderText` in `port/hal/gx/nds_bg_renderer.cpp` (depends on T014): for each of 256 pixels in `scanline`, compute tile row/col from `scrollX/scrollY + bg->screenBase * 2048` with screen-size wrap (256×256 / 512×256 / 256×512 / 512×512), read `GXScrFmtText` entry, call `NdsBg_DecodeTilePixel`, look up RGB555 in `pltt`, convert with `GX_RGB555ToRGBA`; write 0x00000000 for palette index 0
- [X] T016 [US1] Implement `NdsBg_RenderAffine` in `port/hal/gx/nds_bg_renderer.cpp` (depends on T014): per-pixel affine sample using `affineA/B/C/D + affineRefX/Y`, handle `areaOver` wrap/clamp; decode 8bpp affine tile, palette lookup; write transparent for index 0
- [X] T017 [US1] Implement `NdsCompositor_RenderScanline` in `port/hal/gx/nds_compositor.cpp` (depends on T015, T016): collect per-pixel candidates from BG0–BG3 (enabled planes only); dispatch `NdsBg_RenderText` or `NdsBg_RenderAffine` per mode; priority-sort (0=front, 3=back); apply `NdsCompositor_AlphaBlend` if `blendMode == 1`; write winning RGBA8 to `engine->frameBuffer[scanline][x]`; use backdrop (BG palette index 0) when all planes are transparent
- [X] T018 [US1] Implement `NdsGfx_RenderMain` in `port/hal/gx/gx_pipeline.cpp` (depends on T017): loop `y = 0..191`, call `NdsCompositor_RenderScanline(&gNdsGfx.main, y, gHwLcdcVram, gHwOam, (uint16_t*)gHwPltt)`, then call `NdsCompositor_ApplyBrightness(gNdsGfx.main.frameBuffer, gNdsGfx.main.blend.masterBrightness)`
- [X] T019 [US1] Implement `NdsGfx_RenderSub` in `port/hal/gx/gx_pipeline.cpp` (depends on T017): same loop as `NdsGfx_RenderMain` but using `gNdsGfx.sub`, `gHwDbOam`, `(uint16_t*)gHwDbPltt`
- [X] T020 [P] [US1] Implement register scroll synchronization in `port/hal/gx/gx_pipeline.cpp`: decode active scroll offsets (`REG_BG0HOFS`/`VOFS` through `REG_BG3HOFS`/`VOFS` for Main and Sub) directly from `gHardwareIoRegMemory` / `HardwareIoRegisters` at the start of `NdsGfx_RenderMain` / `RenderSub`
- [X] T021 [P] [US1] Implement BG control register synchronization in `port/hal/gx/gx_pipeline.cpp`: decode priority, colorMode, screenBase, charBase, screenSize, and enabled bits from `REG_BG0CNT`..`REG_BG3CNT` in `gHardwareIoRegMemory` into `gNdsGfx.main.bg[]` and `gNdsGfx.sub.bg[]`
- [X] T022 [P] [US1] Implement display control synchronization in `port/hal/gx/gx_pipeline.cpp`: decode `REG_DISPCNT` (visible planes bitmask, bgMode, 3D flag, display mode) from `gHardwareIoRegMemory` into `gNdsGfx.main.dispcnt` and `gNdsGfx.sub.dispcnt`
- [X] T023 [US1] Write unit tests in `port/tests/test_bg_renderer.cpp` (depends on T015, T016, T017): (a) 4bpp text tile decode — 3 known tiles + palette → verify 256×192 RGBA byte-by-byte; (b) scroll wrap at x=255 → seamless tile boundary; (c) affine identity matrix → same output as text mode; (d) disabled BG → all 0x00000000 transparent

**Checkpoint**: Background tilemaps visible; menus / text boxes / title screen art render correctly. `port/tests/test_bg_renderer.cpp` all pass.

---

## Phase 4: User Story 2 — Sprite Objects (OAM) & Palette Blending (Priority: P2)

**Goal**: Render OAM sprites over backgrounds with correct transparency, flip, priority sandwiching, and brightness fade effects.

**Independent Test**: Program OAM entries with known positions/tiles, call `NdsGfx_RenderMain`, verify sprite pixels at exact coordinates with correct palette colors; color 0 pixels yield 0x00000000.

### Implementation for User Story 2

- [X] T024 [US2] Implement `NdsSprite_ParseOam` in `port/hal/gx/nds_sprite_renderer.cpp`: decode all 128 × 8-byte OAM entries from raw `uint8_t*` into `NdsOamEntry[128]`; extract Y, affineFlag, doubleSized, objMode, mosaicEnable, colorMode, shape, X (signed 9-bit), affineIndex, flipH, flipV, size, charName, priority, paletteIndex; compute pixel width/height from shape×size table
- [X] T025 [US2] Implement `NdsSprite_RenderScanline` in `port/hal/gx/nds_sprite_renderer.cpp` (depends on T024): for each of 128 OAM entries, check if `enabled && !affineFlag && objMode != 2`: compute sprite row overlap with `scanline`; for each overlapping x pixel, decode OBJ char data (1D map: `charName * 32 + tileRow * 4`, 8bpp: `charName * 64`, 2D map: `(charName & 0x1F) * 32 + (charName >> 5) * 1024 + tileRow * 4`), apply H/V flip, look up OBJ palette (HW_OBJ_PLTT), write `NdsSpritePixel{color, priority}`; clip at x=0 and x=255 (no wrap); skip color-0 pixels (transparent)
- [X] T026 [US2] Extend `NdsCompositor_RenderScanline` in `port/hal/gx/nds_compositor.cpp` (depends on T025, T017): call `NdsSprite_RenderScanline` to get `NdsSpritePixel[256]`; insert OBJ pixels between BG planes according to OBJ priority P (OBJ at P composites above BG planes with priority > P); apply `NdsCompositor_AlphaBlend` for OBJ mode 1 (semi-transparent)
- [X] T027 [P] [US2] Implement blend control register synchronization in `port/hal/gx/gx_pipeline.cpp`: decode `REG_BLDCNT` and `REG_BLDALPHA` from `gHardwareIoRegMemory` into `gNdsGfx.main.blend.evA/evB/srcMask/dstMask/blendMode`
- [X] T028 [P] [US2] Implement master brightness register synchronization in `port/hal/gx/gx_pipeline.cpp`: decode `REG_MASTER_BRIGHT` from `gHardwareIoRegMemory` into `gNdsGfx.main.blend.masterBrightness` (level bits [4:0], direction bit [14]; positive = lighten, negative = darken)
- [X] T029 [P] [US2] Implement Sub-engine blend and brightness register synchronization in `port/hal/gx/gx_pipeline.cpp`: decode `REG_DB_BLDCNT`, `REG_DB_BLDALPHA`, and `REG_DB_MASTER_BRIGHT` from `gHardwareIoRegMemory` into `gNdsGfx.sub.blend.*` fields
- [X] T030 [US2] Write unit tests in `port/tests/test_sprite_renderer.cpp` (depends on T024, T025, T026): (a) OAM entry at x=10, y=20, 8×8 tile → verify pixels at columns 10–17 on scanline 20; (b) flipH=true → verify mirrored pixel column; (c) sprite at x=250, 16-wide → only pixels 250–255 written; (d) charName pixel index 0 → 0x00000000 (transparent); (e) OBJ priority 1 → sandwiched between BG0 and BG1 in compositor output
- [X] T031 [US2] Write unit tests in `port/tests/test_compositor.cpp` (depends on T012, T013, T017, T025): (a) two BG layers at priorities 0 and 1 → priority 0 wins; (b) alpha blend EVA=8, EVB=8 → verify 50/50 mix per channel (± 1 rounding); (c) `ApplyBrightness +16` → all pixels 0xFFFFFFFF; (d) `ApplyBrightness -16` → all pixels 0xFF000000 (alpha preserved)

**Checkpoint**: Player character, NPCs, menu cursors, and party icons render over backgrounds. Fade to black/white transitions work. All `test_compositor.cpp` and `test_sprite_renderer.cpp` tests pass.

---

## Phase 5: User Story 3 — Fixed-Function 3D Geometry Pipeline & Compositing (Priority: P3)

**Goal**: Render 3D polygon geometry into an FBO, readback into BG0, and composite with 2D planes.

**Independent Test**: Submit a unit triangle with known vertex colors, call `G3_SwapBuffers`, verify non-black RGBA pixels in `gNdsGfx.g3.bg0Readback[]` at the triangle's expected raster position.

### Implementation for User Story 3

- [X] T032 Implement `G3Pipeline_Init` in `port/hal/g3/g3_pipeline.cpp`: create 256×192 FBO — `glGenFramebuffers`, `glGenTextures(GL_RGBA8)`, `glGenRenderbuffers(GL_DEPTH_COMPONENT24)`, `glFramebufferTexture2D`, `glFramebufferRenderbuffer`; check `glCheckFramebufferStatus`; store in `gNdsGfx.g3.fbo/fboColorTex/fboDepthRb`; call from `NdsGfx_Init` (depends on T009)
- [X] T033 Implement `G3Pipeline_Destroy` in `port/hal/g3/g3_pipeline.cpp`: `glDeleteFramebuffers`, `glDeleteTextures`, `glDeleteRenderbuffers` for FBO resources
- [X] T034 [US3] Implement `G3_PushMtx` in `port/hal/g3/g3_pipeline.cpp`: call `glPushMatrix`; increment appropriate depth counter (`projStackDepth` / `posStackDepth` / `texStackDepth`); clamp at max (3/30/0) — no crash on overflow (NDS behavior: status flag only)
- [X] T035 [US3] Implement `G3_PopMtx(int count)` in `port/hal/g3/g3_pipeline.cpp`: call `glPopMatrix` `count` times; decrement counter by `count`; clamp at 0 on underflow
- [X] T036 [P] [US3] Implement `G3_LoadMtx44` in `port/hal/g3/g3_pipeline.cpp`: convert `int32_t[16]` fx32 column-major matrix to `GLfloat[16]` via `FX32_TO_FLOAT`; call `glLoadMatrixf`
- [X] T037 [P] [US3] Implement `G3_MultMtx44` in `port/hal/g3/g3_pipeline.cpp`: same conversion; call `glMultMatrixf`
- [X] T038 [P] [US3] Implement `G3_Perspective` in `port/hal/g3/g3_pipeline.cpp`: compute `glFrustum` parameters from NDS fovySin/fovyCos/aspect (fx32) and near/far (fx32 → float); formula: `top = nearClip * fovySin / fovyCos`, `right = top * aspect`; call `glFrustum(-right, right, -top, top, nearF, farF)`
- [X] T039 [P] [US3] Implement `G3_Ortho` in `port/hal/g3/g3_pipeline.cpp`: convert fx32 parameters to float; call `glOrtho`
- [X] T040 [P] [US3] Implement `G3Pipeline_PolygonAttr` and public `G3_PolygonAttr` wrapper in `port/hal/g3/g3_pipeline.cpp`: map `cullBack/cullFront` → `glCullFace/glEnable(GL_CULL_FACE)` / `glDisable`; `depthTestEnable` → `glEnable/glDisable(GL_DEPTH_TEST)` with `glDepthFunc(GL_LESS)`; `polyAlpha` → store in state (applied as `glColor4f` alpha at next `G3_Begin`)
- [X] T041 [US3] Implement `NdsTexCache_Upload` in `port/hal/g3/nds_texture_cache.cpp`: decode all 7 NDS texture formats to RGBA8 — format 2 (4-color), 3 (16-color), 4 (256-color), 7 (direct RGB555); create GL texture with `glGenTextures/glTexImage2D(GL_RGBA, GL_UNSIGNED_BYTE)`; cache in `std::unordered_map<uint64_t, GLuint>` keyed by `((uint64_t)vramOffset << 32) | paletteOffset`; return cached ID if already present
- [X] T042 [US3] Implement `NdsTexCache_Upload` format decoders (A3I5, A5I3, 4×4-compressed) in `port/hal/g3/nds_texture_cache.cpp` (depends on T041): add decode functions `DecodeFormat_A3I5`, `DecodeFormat_A5I3`, `DecodeFormat_4x4Compressed`; dispatch in format switch; test with known pixel patterns
- [X] T043 [P] [US3] Implement `NdsTexCache_Bind` in `port/hal/g3/nds_texture_cache.cpp`: `glBindTexture(GL_TEXTURE_2D, texId)`; `glEnable(GL_TEXTURE_2D)` if not already
- [X] T044 [P] [US3] Implement `NdsTexCache_Flush` in `port/hal/g3/nds_texture_cache.cpp`: iterate cache map, call `glDeleteTextures`; clear map
- [X] T045 [P] [US3] Implement `G3Pipeline_TexImageParam` and public `G3_TexImageParam` wrapper in `port/hal/g3/g3_pipeline.cpp` (depends on T041): store `texVramOffset, texFormat, texWidth/Height, texRepeatS/T, texEnabled=true`; call `NdsTexCache_Upload`; configure `glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S/T, GL_REPEAT/GL_CLAMP_TO_EDGE)`
- [X] T046 [P] [US3] Implement `G3Pipeline_TexPlttBase` and public `G3_TexPlttBase` wrapper in `port/hal/g3/g3_pipeline.cpp`: store `texPaletteOffset`; invalidate cached texture for current vramOffset (re-upload with new palette on next `G3_Begin`)
- [X] T047 [US3] Implement `G3Pipeline_SwapBuffers` and public `G3_SwapBuffers` wrapper in `port/hal/g3/g3_pipeline.cpp` (depends on T032, T034–T046): bind FBO; `glViewport(0, 0, 256, 192)`; `glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)`; flush any pending `G3_Begin/End` geometry (geometry commands are immediate via OpenGL); `glReadPixels(0, 0, 256, 192, GL_RGBA, GL_UNSIGNED_BYTE, state->bg0Readback)`; flip Y axis (OpenGL origin = bottom-left, NDS origin = top-left); unbind FBO; restore viewport to full window size
- [X] T048 [US3] Wire 3D BG0 readback into compositor in `port/hal/gx/nds_compositor.cpp` (depends on T047, T017): in `NdsCompositor_RenderScanline`, when `engine == &gNdsGfx.main && engine->dispcnt.bg0Is3D`, copy `gNdsGfx.g3.bg0Readback[scanline * 256 + x]` as the BG0 pixel instead of calling `NdsBg_RenderText`
- [X] T049 [US3] Implement BG0 3D mode register synchronization in `port/hal/gx/gx_pipeline.cpp`: synchronize `gNdsGfx.main.dispcnt.bg0Is3D` from `REG_DISPCNT` bit 3 (`DISPCNT_MODE_0_2D3D` / `GX_BG0_AS_3D`)
- [X] T050 [US3] Write unit tests in `port/tests/test_g3_pipeline.cpp` (depends on T032–T048): (a) `G3_PushMtx` × 4 in projection mode → `projStackDepth` clamped at 3, no crash; (b) `G3_PopMtx(1)` from depth 0 → clamp at 0, no crash; (c) `G3Pipeline_SwapBuffers` after submitting solid-color triangle → verify ≥ 1 non-black pixel in `bg0Readback[]` center region (requires headless SDL GL context); (d) `NdsTexCache_Flush` → all GL textures deleted (verify via cache map empty)

**Checkpoint**: 3D geometry renders into FBO; Ho-Oh/Lugia cinematic and battle backgrounds composite with 2D menus. All `test_g3_pipeline.cpp` tests pass.

---

## Phase 6: Polish & Cross-Cutting Concerns

**Purpose**: Hardening, edge cases from spec.md, and performance validation.

- [X] T051 [P] Implement Display Select swap in `port/hal/gx/gx_pipeline.cpp`: verify `NdsGfx_PresentFrame` correctly swaps Main/Sub framebuffers when `gNdsGfx.dispSelect == GX_DISP_SELECT_SUB_MAIN`; write integration smoke test triggering `GX_SetDispSelect` mid-frame
- [X] T052 [P] Implement off-screen sprite clipping hardening in `port/hal/gx/nds_sprite_renderer.cpp`: handle negative X (9-bit signed wrap) sprites that start left of x=0 — render only the visible right portion; handle Y ≥ 192 (wrap at 256-line Y boundary per NDS behavior)
- [X] T053 [P] Implement disabled-plane fast path in `port/hal/gx/nds_compositor.cpp`: when all planes disabled (`visiblePlanes == 0`) emit solid backdrop color; when `dispMode == 0` (display off) emit solid black; skip `NdsBg_Render*` calls for disabled BG layers
- [X] T054 [P] Implement master brightness extremes in `port/hal/gx/nds_compositor.cpp`: verify `+16` → 0xFFFFFF (pure white, alpha preserved); `-16` → 0x000000 (pure black, alpha preserved); no residual pixel leakage
- [X] T055 [P] Implement palette index 0 transparency enforcement in `port/hal/gx/nds_bg_renderer.cpp` and `nds_sprite_renderer.cpp`: audit all palette lookup paths — any index 0 result MUST write `0x00000000` to output, not the RGB value at palette[0]
- [X] T056 [P] Implement matrix stack boundary clamping in `port/hal/g3/g3_pipeline.cpp`: verify `G3_PushMtx` / `G3_PopMtx` depth counter clamp behavior for all 3 stack modes (Projection ≤ 3, Position ≤ 30, Texture ≤ 0); add debug-build `SDL_LogWarn` on overflow/underflow
- [X] T057 Measure frame budget in `port/hal/gx/gx_pipeline.cpp`: add `SDL_GetPerformanceCounter` instrumentation around `NdsGfx_RenderMain` + `NdsGfx_RenderSub`; log to `SDL_Log` when total exceeds 4 ms; verify SC-005 (≤ 16.7 ms full frame) on release build
- [X] T058 [P] Update `port/tests/test_runner.cpp` to discover and run all new test files (`test_bg_renderer.cpp`, `test_sprite_renderer.cpp`, `test_compositor.cpp`, `test_g3_pipeline.cpp`); ensure CI command `./build/port_tests` exits 0 with 100% pass rate (SC-006)
- [X] T059 [P] Update CMakeLists.txt: add `nds_compositor.cpp`, `nds_bg_renderer.cpp`, `nds_sprite_renderer.cpp`, `gx_pipeline.cpp` to the `port` target; add `g3_pipeline.cpp`, `nds_texture_cache.cpp` to the `port` target; add all new test `.cpp` files to `port_tests` target
- [X] T060 [P] Update `specs/003-graphics-pipeline/quickstart.md`: add "Enabling the Real Pipeline" section documenting runtime F9 hotkey toggle and setup for authentic game asset preview mode
- [X] T061 [US1] Implement interactive authentic visual demo in `port/hal/gx/gx_pipeline.cpp` and `port/main.cpp`: load authentic Title Screen / dialogue window tilemaps and palettes from `files/` (e.g. `files/a/0/0/7` or UI NARC graphics) into `gHwLcdcVram` and `gHwPltt` when test pattern is disabled, displaying authentic Pokémon HeartGold 2D & 3D graphics on desktop for manual inspection prior to Phase 5

---

## Dependencies & Execution Order

### Phase Dependencies

```
Phase 1 (Setup)        → No dependencies — start immediately
Phase 2 (Foundational) → Depends on Phase 1 (T001–T008 complete)
Phase 3 (US1 / P1)    → Depends on Phase 2 — starts after T013 complete
Phase 4 (US2 / P2)    → Depends on Phase 2 — may start alongside Phase 3
Phase 5 (US3 / P3)    → Depends on Phase 2; T048 also depends on T017 (Phase 3)
Phase 6 (Polish)       → Depends on all prior phases
```

### User Story Dependencies

- **US1 (P1) BG Tilemaps**: Foundational phase complete → no other story dependency
- **US2 (P2) OAM Sprites**: Foundational phase complete → depends on US1 compositor (T017) for integration task T026 only; `NdsSprite_ParseOam/RenderScanline` (T024, T025) are fully independent
- **US3 (P3) 3D Pipeline**: Foundational + FBO init (T032) → 3D tasks T034–T046 fully independent of US1/US2; T048 (BG0 readback wiring) depends on T017 (compositor scanline)

### Within Each Phase

```
US1: T014 [P] → T015 → T016 [P with T015] → T017 → T018/T019 → T020/T021/T022 [P] → T023
US2: T024 → T025 → T026 (needs T017) | T027/T028/T029 [P with T025] → T030/T031
US3: T032 → T034–T040 [some P] → T041 → T042–T046 [P] → T047 → T048 → T049 → T050
```

### Parallel Opportunities

| Opportunity | Tasks |
|-------------|-------|
| All Phase 1 headers (except T001/T002 which are shared types) | T003, T004, T005, T006, T007, T008 |
| Phase 2 blend/brightness implementations | T012, T013 (independent files) |
| US1: tile decode and affine | T014 (decoder) enables T015/T016 in parallel |
| US1: register shims | T020, T021, T022 (independent functions) |
| US2: OAM parse independent of compositor | T024, T025 (before T026) |
| US2: register shims | T027, T028, T029 |
| US3: matrix ops | T034, T035, T036, T037, T038, T039, T040 (independent) |
| US3: texture cache | T041 → then T042, T043, T044 in parallel |
| Phase 6: all polish tasks | T051–T060 mostly independent |

---

## Parallel Example: User Story 1 (BG Tilemap Rendering)

```
# Step 1 — Parallel (once T001 header exists):
Task T014: "Implement NdsBg_DecodeTilePixel in port/hal/gx/nds_bg_renderer.cpp"

# Step 2 — Parallel (once T014 is done):
Task T015: "Implement NdsBg_RenderText ..."
Task T016: "Implement NdsBg_RenderAffine ..."

# Step 3 — Sequential (needs T015 + T016):
Task T017: "Implement NdsCompositor_RenderScanline ..."

# Step 4 — Parallel (once T017 is done):
Task T018: "Implement NdsGfx_RenderMain ..."
Task T019: "Implement NdsGfx_RenderSub ..."
Task T020: "Add G2_SetBGxOffset shims ..."
Task T021: "Add G2_SetBGxControl shims ..."
Task T022: "Add GX_SetVisiblePlane shim ..."
```

---

## Implementation Strategy

### MVP First (User Story 1 Only)

1. Complete **Phase 1** (Setup headers): T001–T008
2. Complete **Phase 2** (Pipeline skeleton): T009–T013
3. Complete **Phase 3** (BG Tilemap): T014–T023
4. **STOP and VALIDATE**: menus, text boxes, and overworld map render correctly
5. Ship MVP — game is playable with 2D backgrounds

### Incremental Delivery

1. Setup + Foundational → black screen (pipeline wired, test pattern disabled)
2. US1 (Phase 3) → BG tilemap rendering: title screen, menus, overworld tiles visible 🎯
3. US2 (Phase 4) → OAM sprites: player, NPCs, cursors, icons, fade effects
4. US3 (Phase 5) → 3D geometry: buildings, cinematic sequences, battle stages
5. Phase 6 → Edge case hardening and performance validation

### Parallel Team Strategy

With two developers:

- **Developer A** (Phases 1→3): Setup headers → Pipeline skeleton → BG Renderer → Compositor
- **Developer B** (Phase 5 early start): G3 FBO setup (T032) → Matrix stack (T034–T040) → Texture cache (T041–T046) can begin in parallel with Phase 3; T047–T049 blocked only on T017

---

## Notes

- `[P]` tasks operate on different files or independent functions — safe to run in parallel
- `[US1/2/3]` label maps each task to its user story for traceability
- Each phase ends with an independently verifiable checkpoint
- Keep `GX_SetTestPatternEnabled(true)` as the default until Phase 3 checkpoint passes
- All `#ifdef PLATFORM_NATIVE` guards in register shims (T020–T022, T027–T029, T049) prevent DS build breakage
- Commit after each logical group (e.g., after T017, after T024–T025, after T032)
- Total tasks: **61** | Phase 1: 8 | Phase 2: 5 | US1: 10 | US2: 8 | US3: 19 | Polish: 11
