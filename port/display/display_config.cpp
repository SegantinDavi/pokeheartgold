/**
 * @file display_config.cpp
 * @brief Implementation of display preferences persistence (saving/loading .ini configuration).
 */

#include "display_config.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string>

extern "C" {

void DisplayConfig_SetDefaults(DisplayConfig* config) {
    if (!config) return;
    memset(config, 0, sizeof(DisplayConfig));
    config->mode = DISPLAY_LAYOUT_HORIZONTAL_DEFAULT;
    config->windowWidth = 1280;
    config->windowHeight = 720;
    config->isFullscreen = false;
    config->vsyncEnabled = true;
}

bool DisplayConfig_Load(DisplayConfig* config, const char* filePath) {
    if (!config) return false;
    DisplayConfig_SetDefaults(config);

    const char* path = filePath ? filePath : DEFAULT_DISPLAY_CONFIG_PATH;
    FILE* f = fopen(path, "r");
    if (!f) {
        return false;
    }

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        char key[64];
        int val = 0;
        if (sscanf(line, " %63[^= ] = %d", key, &val) == 2) {
            if (strcmp(key, "mode") == 0) {
                if (val >= 0 && val < DISPLAY_LAYOUT_COUNT) {
                    config->mode = static_cast<DisplayLayoutMode>(val);
                }
            } else if (strcmp(key, "width") == 0) {
                if (val >= 256) {
                    config->windowWidth = val;
                }
            } else if (strcmp(key, "height") == 0) {
                if (val >= 192) {
                    config->windowHeight = val;
                }
            } else if (strcmp(key, "fullscreen") == 0) {
                config->isFullscreen = (val != 0);
            } else if (strcmp(key, "vsync") == 0) {
                config->vsyncEnabled = (val != 0);
            }
        }
    }

    fclose(f);
    return true;
}

bool DisplayConfig_Save(const DisplayConfig* config, const char* filePath) {
    if (!config) return false;

    const char* path = filePath ? filePath : DEFAULT_DISPLAY_CONFIG_PATH;
    std::string tempPath = std::string(path) + ".tmp";

    FILE* f = fopen(tempPath.c_str(), "w");
    if (!f) {
        return false;
    }

    fprintf(f, "# Pokémon HeartGold Native PC Port Display Configuration\n");
    fprintf(f, "[Display]\n");
    fprintf(f, "mode = %d\n", static_cast<int>(config->mode));
    fprintf(f, "width = %d\n", config->windowWidth);
    fprintf(f, "height = %d\n", config->windowHeight);
    fprintf(f, "fullscreen = %d\n", config->isFullscreen ? 1 : 0);
    fprintf(f, "vsync = %d\n", config->vsyncEnabled ? 1 : 0);

    fclose(f);

    // Atomic rename
    return (rename(tempPath.c_str(), path) == 0);
}

} // extern "C"
