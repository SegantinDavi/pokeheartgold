/**
 * @file port_common.h
 * @brief Common definitions, constants, and types for the native PC port.
 */

#ifndef PORT_COMMON_H
#define PORT_COMMON_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* --- Nintendo DS Screen Dimensions & Timing --- */
#define NDS_SCREEN_WIDTH       256
#define NDS_SCREEN_HEIGHT      192
#define NDS_SCREEN_TOTAL_PIXELS (NDS_SCREEN_WIDTH * NDS_SCREEN_HEIGHT)
#define NDS_FRAME_RATE         59.8261
#define NDS_FRAME_TIME_MS      16.715
#define NDS_AUDIO_SAMPLE_RATE  32768
#define NDS_SAVE_FILE_SIZE     (512 * 1024) /* 512 KB standard DS Flash backup */

/* --- Display Layout Modes --- */
typedef enum {
    DISPLAY_LAYOUT_HORIZONTAL_DEFAULT = 0, /* Top Screen Left, Touchscreen Right (Default) */
    DISPLAY_LAYOUT_HORIZONTAL_SWAPPED = 1, /* Touchscreen Left, Top Screen Right */
    DISPLAY_LAYOUT_VERTICAL_CLASSIC   = 2, /* Top Screen Above, Touchscreen Below */
    DISPLAY_LAYOUT_VERTICAL_SWAPPED   = 3, /* Touchscreen Above, Top Screen Below */
    DISPLAY_LAYOUT_COUNT
} DisplayLayoutMode;

/* --- Viewport Bounding Box --- */
typedef struct {
    int32_t x;
    int32_t y;
    int32_t width;
    int32_t height;
} ViewportRect;

/* --- Utility Helper Macros --- */
#ifndef PORT_MIN
#define PORT_MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef PORT_MAX
#define PORT_MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef PORT_CLAMP
#define PORT_CLAMP(val, min_val, max_val) \
    (((val) < (min_val)) ? (min_val) : (((val) > (max_val)) ? (max_val) : (val)))
#endif

#ifndef PORT_ARRAY_SIZE
#define PORT_ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif

#ifndef PORT_UNUSED
#define PORT_UNUSED(var) ((void)(var))
#endif

#ifdef __cplusplus
}
#endif

#endif /* PORT_COMMON_H */
