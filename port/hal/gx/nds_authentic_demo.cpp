/**
 * @file nds_authentic_demo.cpp
 * @brief Authentic Pokémon HeartGold Title Screen and graphical asset preview demo.
 */

#define GL_GLEXT_PROTOTYPES 1
#include "nds_authentic_demo.h"
#include "gx_pipeline.h"
#include "g3_pipeline.h"
#include "port_gx.h"
#include "port_hw_registers.h"
#include <SDL.h>
#include <SDL_opengl.h>
#include <cstdio>
#include <cstring>
#include <vector>
#include <string>
#include <cmath>

namespace {

bool s_assetsLoaded = false;

std::string FindAssetPath(const std::string& relativePath) {
    const char* prefixes[] = {
        "",
        "../",
        "../../"
    };
    for (const char* p : prefixes) {
        std::string candidate = std::string(p) + relativePath;
        FILE* f = fopen(candidate.c_str(), "rb");
        if (f) {
            fclose(f);
            return candidate;
        }
    }
    char* basePath = SDL_GetBasePath();
    if (basePath) {
        std::string candidate = std::string(basePath) + relativePath;
        SDL_free(basePath);
        FILE* f = fopen(candidate.c_str(), "rb");
        if (f) {
            fclose(f);
            return candidate;
        }
    }
    return relativePath;
}

bool ReadFileData(const std::string& path, size_t headerSkip, void* dst, size_t maxBytes) {
    std::string fullPath = FindAssetPath(path);
    FILE* f = fopen(fullPath.c_str(), "rb");
    if (!f) return false;
    if (headerSkip > 0) {
        fseek(f, static_cast<long>(headerSkip), SEEK_SET);
    }
    size_t read = fread(dst, 1, maxBytes, f);
    fclose(f);
    return read > 0;
}

} // anonymous namespace

extern "C" {

bool NdsAuthenticDemo_Init(void) {
    if (!s_assetsLoaded) {
        // Load authentic Pokémon HeartGold Title Screen graphics from files/demo/title/titledemo/
        // 1. Palette: titledemo_00000002.NCLR (512 bytes raw palette at offset 40)
        std::string nclrPath = "files/demo/title/titledemo/titledemo_00000002.NCLR";
        ReadFileData(nclrPath, 40, gHwPltt, 512);
        ReadFileData(nclrPath, 40, gHwDbPltt, 512);

        // Also populate OBJ palette with contrasting vivid colors for interactive sprites
        uint16_t* objPltt = reinterpret_cast<uint16_t*>(gHwPltt + 512);
        uint16_t* dbObjPltt = reinterpret_cast<uint16_t*>(gHwDbPltt + 512);
        objPltt[1] = 0x001F; // Red
        objPltt[2] = 0x7FE0; // Gold / Yellow
        objPltt[3] = 0x7FFF; // White
        dbObjPltt[1] = 0x001F;
        dbObjPltt[2] = 0x7FE0;
        dbObjPltt[3] = 0x7FFF;

        // 2. Main screen Title Logo tiles: titledemo_00000001.NCGR (30720 bytes at offset 48)
        std::string ncgrMainPath = "files/demo/title/titledemo/titledemo_00000001.NCGR";
        ReadFileData(ncgrMainPath, 48, gHwLcdcVram + 0, 30720);

        // 3. Main screen Title Logo screen map: titledemo_00000000.NSCR (2048 bytes at offset 36)
        std::string nscrMainPath = "files/demo/title/titledemo/titledemo_00000000.NSCR";
        ReadFileData(nscrMainPath, 36, gHwLcdcVram + 0x8000, 2048);

        // 4. Sub screen bottom artwork: titledemo_00000036.NCGR (offset 48) and 00000037.NSCR (offset 36)
        std::string ncgrSubPath = "files/demo/title/titledemo/titledemo_00000036.NCGR";
        ReadFileData(ncgrSubPath, 48, gHwLcdcVram + 0x10000, 49152);

        std::string nscrSubPath = "files/demo/title/titledemo/titledemo_00000037.NSCR";
        ReadFileData(nscrSubPath, 36, gHwLcdcVram + 0x1C000, 2048);

        // 5. Create a stylized 8x8 Pokéball sprite tile in OBJ VRAM (offset 0x20000)
        uint8_t* objVram = gHwLcdcVram + 0x20000;
        std::memset(objVram, 0, 32);
        // Draw 8x8 circular Pokéball: top red (1), bottom white (3), center ring (2)
        const uint8_t pokeball[8][8] = {
            {0, 0, 1, 1, 1, 1, 0, 0},
            {0, 1, 1, 1, 1, 1, 1, 0},
            {1, 1, 1, 1, 1, 1, 1, 1},
            {2, 2, 2, 3, 3, 2, 2, 2},
            {2, 2, 2, 3, 3, 2, 2, 2},
            {3, 3, 3, 3, 3, 3, 3, 3},
            {0, 3, 3, 3, 3, 3, 3, 0},
            {0, 0, 3, 3, 3, 3, 0, 0}
        };
        for (int y = 0; y < 8; ++y) {
            for (int x = 0; x < 8; x += 2) {
                uint8_t p0 = pokeball[y][x];
                uint8_t p1 = pokeball[y][x + 1];
                objVram[y * 4 + (x / 2)] = (p1 << 4) | (p0 & 0x0F);
            }
        }
        s_assetsLoaded = true;
    }

    // Configure Display Engines:
    // Main Engine (Top screen):
    // BG0: 3D Geometry pass (Ho-Oh / Sacred Fire 3D space)
    // BG1: Authentic 2D Title Logo
    // OBJ: Particle effects
    gNdsGfx.main.dispcnt.dispMode = 1;
    gNdsGfx.main.dispcnt.bgMode = 0;
    gNdsGfx.main.dispcnt.visiblePlanes = (1 << 0) | (1 << 1) | (1 << 4);
    gNdsGfx.main.dispcnt.bg0Is3D = true;
    gNdsGfx.main.dispcnt.objMapChar1D = true;

    gNdsGfx.main.bg[0].enabled = true;
    gNdsGfx.main.bg[0].priority = 2; // Behind Title Logo

    gNdsGfx.main.bg[1].enabled = true;
    gNdsGfx.main.bg[1].mode = NDS_BGMODE_TEXT;
    gNdsGfx.main.bg[1].colorMode = 1; // 8bpp
    gNdsGfx.main.bg[1].priority = 0; // Front
    gNdsGfx.main.bg[1].charBase = 0; // 0x00000
    gNdsGfx.main.bg[1].screenBase = 16; // 16 * 2048 = 0x8000
    gNdsGfx.main.bg[1].screenSize = 0; // 256x256
    gNdsGfx.main.bg[1].scrollX = 0;
    gNdsGfx.main.bg[1].scrollY = 0;

    // Sub Engine (Touchscreen):
    // BG1: Authentic Touchscreen Background Art
    // OBJ: Stylus touch cursor
    gNdsGfx.sub.dispcnt.dispMode = 1;
    gNdsGfx.sub.dispcnt.bgMode = 0;
    gNdsGfx.sub.dispcnt.visiblePlanes = (1 << 1) | (1 << 4);
    gNdsGfx.sub.dispcnt.objMapChar1D = true;

    gNdsGfx.sub.bg[1].enabled = true;
    gNdsGfx.sub.bg[1].mode = NDS_BGMODE_TEXT;
    gNdsGfx.sub.bg[1].colorMode = 1; // 8bpp
    gNdsGfx.sub.bg[1].priority = 1;
    gNdsGfx.sub.bg[1].charBase = 4; // 4 * 16384 = 0x10000
    gNdsGfx.sub.bg[1].screenBase = 56; // 56 * 2048 = 0x1C000
    gNdsGfx.sub.bg[1].screenSize = 0;
    gNdsGfx.sub.bg[1].scrollX = 0;
    gNdsGfx.sub.bg[1].scrollY = 0;

    return true;
}

void NdsAuthenticDemo_RenderFrame(int frameCount, int touchX, int touchY, bool touching) {
    if (!s_assetsLoaded) {
        NdsAuthenticDemo_Init();
    }

    // 1. Update Sub-screen OAM Sprite (Touch cursor Pokéball following stylus)
    uint16_t* subOam = reinterpret_cast<uint16_t*>(gHwDbOam);
    int spriteX = touching ? touchX - 4 : 124;
    int spriteY = touching ? touchY - 4 : 140 + static_cast<int>(std::sin(frameCount * 0.1) * 6.0);

    // Clamp inside viewport
    if (spriteX < 0) spriteX = 0;
    if (spriteX > 248) spriteX = 248;
    if (spriteY < 0) spriteY = 0;
    if (spriteY > 184) spriteY = 184;

    subOam[0] = static_cast<uint16_t>(spriteY & 0xFF); // Attr0
    subOam[1] = static_cast<uint16_t>(spriteX & 0x1FF); // Attr1 (size=0, 8x8)
    subOam[2] = 0; // Attr2 (charName=0, priority=0)

    // Disable remaining sprites
    for (int i = 1; i < 128; ++i) {
        subOam[i * 4 + 0] = (1 << 9);
    }

    // 2. Render 3D Geometry Pass into FBO (Ho-Oh Sacred Gold 3D prism and sparkling celestial field)
    if (SDL_GL_GetCurrentContext() != nullptr) {
        if (gNdsGfx.g3.fbo == 0) {
            G3Pipeline_Init(&gNdsGfx.g3);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, gNdsGfx.g3.fbo);
        glViewport(0, 0, 256, 192);

        // Deep twilight sky background
        glClearColor(0.04f, 0.04f, 0.10f, 1.0f);
        glClearDepth(1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glFrustum(-1.33f * 0.1f, 1.33f * 0.1f, -0.1f, 0.1f, 0.1f, 50.0f);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glTranslatef(0.0f, 0.2f, -3.2f);

        // Rotation angles based on frame count
        float angleY = frameCount * 1.5f;
        float angleX = std::sin(frameCount * 0.03f) * 15.0f;
        glRotatef(angleX, 1.0f, 0.0f, 0.0f);
        glRotatef(angleY, 0.0f, 1.0f, 0.0f);

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glDisable(GL_TEXTURE_2D);

        // Draw 3D Golden Ho-Oh Sacred Emblem / Octahedron
        glBegin(GL_TRIANGLES);

        // Top pyramid (Gold/Amber shades)
        // Face 1: Front
        glColor4f(1.0f, 0.85f, 0.2f, 1.0f); // Bright Gold
        glVertex3f( 0.0f,  0.8f,  0.0f);
        glVertex3f(-0.6f,  0.0f,  0.6f);
        glVertex3f( 0.6f,  0.0f,  0.6f);

        // Face 2: Right
        glColor4f(0.9f, 0.70f, 0.15f, 1.0f); // Warm Gold
        glVertex3f( 0.0f,  0.8f,  0.0f);
        glVertex3f( 0.6f,  0.0f,  0.6f);
        glVertex3f( 0.6f,  0.0f, -0.6f);

        // Face 3: Back
        glColor4f(0.8f, 0.60f, 0.10f, 1.0f); // Deep Gold
        glVertex3f( 0.0f,  0.8f,  0.0f);
        glVertex3f( 0.6f,  0.0f, -0.6f);
        glVertex3f(-0.6f,  0.0f, -0.6f);

        // Face 4: Left
        glColor4f(0.95f, 0.75f, 0.20f, 1.0f);
        glVertex3f( 0.0f,  0.8f,  0.0f);
        glVertex3f(-0.6f,  0.0f, -0.6f);
        glVertex3f(-0.6f,  0.0f,  0.6f);

        // Bottom pyramid (Silver/Crimson accents)
        // Face 5: Front bottom
        glColor4f(0.9f, 0.2f, 0.15f, 1.0f); // Sacred Crimson
        glVertex3f( 0.0f, -0.8f,  0.0f);
        glVertex3f( 0.6f,  0.0f,  0.6f);
        glVertex3f(-0.6f,  0.0f,  0.6f);

        // Face 6: Right bottom
        glColor4f(0.75f, 0.15f, 0.10f, 1.0f);
        glVertex3f( 0.0f, -0.8f,  0.0f);
        glVertex3f( 0.6f,  0.0f, -0.6f);
        glVertex3f( 0.6f,  0.0f,  0.6f);

        // Face 7: Back bottom
        glColor4f(0.85f, 0.85f, 0.95f, 1.0f); // Silver feather accent
        glVertex3f( 0.0f, -0.8f,  0.0f);
        glVertex3f(-0.6f,  0.0f, -0.6f);
        glVertex3f( 0.6f,  0.0f, -0.6f);

        // Face 8: Left bottom
        glColor4f(0.80f, 0.18f, 0.12f, 1.0f);
        glVertex3f( 0.0f, -0.8f,  0.0f);
        glVertex3f(-0.6f,  0.0f,  0.6f);
        glVertex3f(-0.6f,  0.0f, -0.6f);

        glEnd();

        // Draw spinning sparkle particles
        glBegin(GL_QUADS);
        for (int p = 0; p < 8; ++p) {
            float pAngle = (frameCount * 2.0f + p * 45.0f) * 0.0174533f;
            float pDist = 1.2f + 0.3f * std::sin(frameCount * 0.05f + p);
            float px = std::cos(pAngle) * pDist;
            float py = std::sin(frameCount * 0.08f + p) * 0.4f;
            float pz = std::sin(pAngle) * pDist;

            float sz = 0.04f;
            glColor4f(1.0f, 1.0f, 0.7f, 0.9f);
            glVertex3f(px - sz, py - sz, pz);
            glVertex3f(px + sz, py - sz, pz);
            glVertex3f(px + sz, py + sz, pz);
            glVertex3f(px - sz, py + sz, pz);
        }
        glEnd();

        glDisable(GL_DEPTH_TEST);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // Perform swap and readback into BG0
        G3Pipeline_SwapBuffers(&gNdsGfx.g3);
    }
}

} // extern "C"
