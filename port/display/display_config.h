/**
 * @file display_config.h
 * @brief Display preference persistence for layout mode and window size.
 */

#ifndef PORT_DISPLAY_CONFIG_H
#define PORT_DISPLAY_CONFIG_H

#include "display_manager.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DEFAULT_DISPLAY_CONFIG_PATH "display_config.ini"

bool DisplayConfig_Load(DisplayConfig* config, const char* filePath);
bool DisplayConfig_Save(const DisplayConfig* config, const char* filePath);
void DisplayConfig_SetDefaults(DisplayConfig* config);

#ifdef __cplusplus
}
#endif

#endif // PORT_DISPLAY_CONFIG_H
