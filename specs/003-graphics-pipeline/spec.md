# Feature Specification: 2D & 3D Graphics Pipeline Emulation

**Feature Branch**: `003-graphics-pipeline`  
**Created**: 2026-11-09  
**Status**: Ready for Planning  
**Input**: Phase 4 of Roadmap: 2D Graphics Engine (`GX_`, `G2_`) and 3D Graphics Engine (`G3_`, `G3X_`) native pipeline emulation and rendering.

---

## 1. Executive Summary

This feature implements the native graphics rendering pipeline for the Pokémon HeartGold desktop recompilation. It replaces the placeholder test pattern with an authentic graphics emulation engine capable of rendering Nintendo DS 2D background tilemaps, animated sprite objects (OAM), color palette blending, and fixed-function 3D geometry directly into the dual-screen viewports at native 59.8261 Hz frame rates.

---

## 2. User Scenarios & Testing *(mandatory)*

### User Story 1 - 2D Background Tilemap Rendering (Priority: P1)

As a player, I want to see crisp, authentic 2D background graphics (dialog boxes, menus, overworld maps, and user interfaces) rendered accurately across both screens so that the game's environments and text are clearly readable and properly composed.

**Why this priority**: 2D backgrounds constitute the visual bedrock of Pokémon HeartGold, forming the entirety of menus, dialogue boxes, status screens, and overworld terrain tiles. Without background rendering, no in-game text or environment can be displayed.

**Independent Test**: Can be tested by loading and displaying sample background tilemaps (e.g., text box borders, menu frames, title screen artwork) onto the Top and Sub screen viewports and verifying that character tiles, screen bases, and palette colors match authentic visual output without distortion or missing tiles.

**Acceptance Scenarios**:

1. **Given** valid tile character data and screen map blocks loaded into virtual display memory, **When** a background layer (BG0-BG3) is enabled with text mode, **Then** the screen renders the corresponding 8x8 tiles with their assigned 16-color or 256-color palettes at the specified coordinates.
2. **Given** an active background layer with horizontal and vertical scroll offsets applied, **When** the frame renders, **Then** the background image shifts smoothly across the viewport matching the exact pixel offsets without seam artifacts.
3. **Given** multiple active background layers with assigned priorities (0 through 3), **When** layers overlap, **Then** lower-numbered priority layers are rendered in front of higher-numbered priority layers.
4. **Given** the display select register is configured, **When** switched between standard and swapped modes, **Then** the Main and Sub background engines route to the designated viewports (Top and Touchscreen) without corruption.

---

### User Story 2 - Sprite Objects (OAM) & Palette Blending (Priority: P2)

As a player, I want animated characters, Pokémon icons, menu cursors, and visual effects to render cleanly on top of backgrounds with transparent boundaries and smooth color fading transitions so that the visual presentation looks indistinguishable from the authentic handheld experience.

**Why this priority**: Sprites represent interactive game elements (the player character, NPCs, menu arrows, Pokémon party icons). Color blending is necessary for daylight cycles, battle transitions, and fading to black/white during map warps.

**Independent Test**: Can be tested by loading OAM attributes and sprite graphic patterns over an active background, verifying that sprites render with correct shapes/sizes, color 0 is completely transparent, layer priorities correctly sandwich sprites between background planes, and brightness modification transitions apply evenly.

**Acceptance Scenarios**:

1. **Given** sprite attributes defined in Object Attribute Memory (OAM), **When** rendered, **Then** sprites appear at their configured coordinates in square (8x8 to 64x64), wide, or tall configurations.
2. **Given** a sprite rendered over a background, **When** pixel color 0 is encountered in the sprite pattern, **Then** the pixel is treated as transparent and reveals the underlying background plane.
3. **Given** sprite attributes with horizontal or vertical flip flags enabled, **When** rendered, **Then** the sprite imagery mirrors accordingly across the appropriate axis.
4. **Given** screen brightness transition commands (fade to black or fade to white), **When** brightness is modulated, **Then** all colors on the targeted screen darken or lighten proportionally across frames.
5. **Given** alpha blending configured between two planes, **When** overlapping pixels are composed, **Then** colors are blended according to the specified source and destination weighting factors.

---

### User Story 3 - Fixed-Function 3D Geometry Pipeline & Compositing (Priority: P3)

As a player, I want 3D town buildings, cinematic intro sequences, and dynamic battle stage elements to render with correct perspective, texture mapping, and depth sorting, integrated seamlessly alongside the 2D background planes.

**Why this priority**: While the majority of the UI is 2D, Pokémon HeartGold utilizes the Nintendo DS 3D graphics hardware for overworld structures, cinematic sequences (title screen Ho-Oh/Lugia, Arceus intro), and battle environments. 

**Independent Test**: Can be tested by submitting 3D geometry commands (matrix transformations, vertex declarations, polygon attributes, texture mapping) and verifying that rasterized 3D geometry composites directly into the Main display plane with proper depth testing and lighting.

**Acceptance Scenarios**:

1. **Given** projection and modelview matrices set in the 3D pipeline, **When** vertices are submitted, **Then** 3D polygons are correctly transformed and perspective-projected onto the viewport.
2. **Given** textured polygons with UV texture coordinates and palette references, **When** rasterized, **Then** textures are mapped onto polygon faces with correct wrapping and filtering.
3. **Given** overlapping polygons at differing depth values, **When** depth testing is enabled, **Then** closer polygons occlude farther polygons without Z-fighting or depth inversion.
4. **Given** the 3D engine configured to output to the Main screen background layer (BG0), **When** the composite frame is generated, **Then** 3D visual output is layered with 2D planes according to priority settings.

---

## 3. Edge Cases

- **Display Selection Swapping**: When the game dynamically toggles `GX_SetDispSelect` (e.g. during Pokégear activation or menu opening), the Main 2D/3D engine and Sub 2D engine must instantly swap viewport destinations without frame flickering or tearing.
- **Off-Screen Sprite Wrapping**: Sprites positioned partially off-screen (negative X/Y coordinates using Nintendo DS sign extension) must clip cleanly at screen boundaries without wrapping around to the opposite side.
- **Empty / Disabled Planes**: Any background or sprite plane marked disabled in display control registers must consume zero rendering time and composite as transparent backdrop.
- **Master Brightness Extremes**: When brightness is set to maximum white (+16) or maximum black (-16), the viewport must display a solid uniform color with zero residual pixel leakage.
- **Palette Index 0 Transparency**: Across all 4bpp (16-color) and 8bpp (256-color) modes, index 0 must strictly act as a transparent pass-through to lower layers, never rendering opaque color unless used as the screen backdrop.
- **Matrix Stack Boundaries**: Overflow or underflow attempts on the 3D matrix stacks (4 projection, 31 modelview, 3 texture) must clamp safely without corrupting host application memory.

---

## 4. Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: System MUST emulate background planes BG0, BG1, BG2, and BG3 for both the Main and Sub graphics engines.
- **FR-002**: System MUST support standard 4bpp (16 palettes of 16 colors) and 8bpp (1 palette of 256 colors) tilemap decoding with 8x8 character tiles.
- **FR-003**: System MUST support individual horizontal and vertical pixel scrolling for all active background layers.
- **FR-004**: System MUST emulate Object Attribute Memory (OAM), supporting up to 128 hardware sprites per screen with square, wide, and tall aspect ratios up to 64x64 pixels.
- **FR-005**: System MUST support sprite attribute flags including horizontal flip, vertical flip, mosaic, and palette selection.
- **FR-006**: System MUST layer 2D backgrounds and sprites in accordance with 4-level hardware priority ordering (0 highest, 3 lowest).
- **FR-007**: System MUST provide color palette management converting Nintendo DS 15-bit RGB555 colors to host 32-bit RGBA pixel formats.
- **FR-008**: System MUST support master brightness adjustment (-16 black fade to +16 white fade) for screen transition effects.
- **FR-009**: System MUST support two-plane alpha blending with configurable source and destination blend weights.
- **FR-010**: System MUST emulate the fixed-function 3D vertex pipeline, including matrix stack operations (Projection, Coordinate, Texture).
- **FR-011**: System MUST rasterize 3D polygonal primitives (triangles, quads, triangle strips) with perspective-correct interpolation, depth testing, and culling.
- **FR-012**: System MUST map 2D textures onto 3D polygons with support for texture coordinates and texture palette lookups.
- **FR-013**: System MUST composite 3D rendered output directly onto the designated Main screen layer alongside 2D planes.
- **FR-014**: System MUST route composited Main and Sub screen outputs to the top and touchscreen viewports based on the active display selection mode.

---

### Key Entities

- **`BackgroundLayer`**: State and configuration for an individual 2D background plane (BG0-BG3), including mode (text/affine), screen base block, character base block, scroll offsets, palette assignment, and layer priority.
- **`SpriteObject`**: An OAM entry defining an individual visual object, containing screen position (X, Y), shape, size, priority, flip flags, palette index, and character name offset.
- **`GraphicsPalette`**: 256-entry color lookup table storing RGB555 color definitions for background and sprite layers, with dedicated transparency handling for color 0.
- **`DisplayCompositor`**: Core rendering engine that evaluates priorities across all 4 background planes, sprite objects, window masks, and 3D capture buffers, writing the final composed 256x192 RGBA image for each display.
- **`MatrixStack`**: 3D fixed-function transformation stack maintaining 4x4 matrix hierarchies for perspective projection, modelview coordinate space, and texture coordinate mapping.
- **`PolygonPrimitive`**: 3D geometric unit comprising vertices, normals, UV coordinates, color attributes, polygon mode, and depth buffer values.

---

## 5. Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: Background tilemaps render with 100% pixel-accurate character positioning and palette mapping against reference game visuals.
- **SC-002**: Object sprites render with complete transparency for color 0, zero border artifacts, and proper depth layering between background planes.
- **SC-003**: Screen brightness adjustments (fade to black / fade to white) transition smoothly across 16 discrete gradation levels without color banding or visual glitches.
- **SC-004**: 3D geometry renders with perspective-correct projection, valid depth occlusion, and clean polygon rasterization without Z-fighting.
- **SC-005**: The combined 2D and 3D graphics rendering pipeline executes within a 16.7 ms frame budget, sustaining a steady 59.8261 Hz frame rate on desktop hardware.
- **SC-006**: Automated visual regression and pipeline unit tests achieve 100% pass rate in the continuous test suite.
