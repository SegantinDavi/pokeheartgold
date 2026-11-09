/**
 * @file port_hw_registers.h
 * @brief Virtual hardware memory arena and emulated Nintendo DS I/O registers.
 */

#ifndef PORT_HW_REGISTERS_H
#define PORT_HW_REGISTERS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <nitro/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NDS_MAIN_RAM_SIZE    (4 * 1024 * 1024)   /* 4 MB Nintendo DS Main RAM */
#define NDS_IO_REG_SIZE      (0x2000)            /* 8 KB I/O register window */
#define NDS_VRAM_BUFFER_SIZE (512 * 1024)        /* 512 KB VRAM */

#define HW_LCDC_VRAM_SIZE    (656 * 1024)
#define HW_OAM_SIZE          (1024)
#define HW_DB_OAM_SIZE       (1024)
#define HW_PLTT_SIZE         (1024)
#define HW_DB_PLTT_SIZE      (1024)

extern uint8_t gHwLcdcVram[HW_LCDC_VRAM_SIZE];
extern uint8_t gHwOam[HW_OAM_SIZE];
extern uint8_t gHwDbOam[HW_DB_OAM_SIZE];
extern uint8_t gHwPltt[HW_PLTT_SIZE];
extern uint8_t gHwDbPltt[HW_DB_PLTT_SIZE];

#define HW_LCDC_VRAM ((uintptr_t)gHwLcdcVram)
#define HW_OAM       ((uintptr_t)gHwOam)
#define HW_DB_OAM    ((uintptr_t)gHwDbOam)
#define HW_PLTT      ((uintptr_t)gHwPltt)
#define HW_DB_PLTT   ((uintptr_t)gHwDbPltt)
#define HW_BG_PLTT   HW_PLTT
#define HW_OBJ_PLTT  (HW_PLTT + 0x200)
#define HW_DB_BG_PLTT HW_DB_PLTT
#define HW_DB_OBJ_PLTT (HW_DB_PLTT + 0x200)

#ifdef PLATFORM_NATIVE
#define HW_REG_BASE   ((uintptr_t)gHardwareIoRegMemory)
#define HW_IOREG      HW_REG_BASE
#define HW_BG_VRAM    ((uintptr_t)gHwLcdcVram)
#endif

typedef struct HardwareIoRegisters {
    uint32_t dispCntMain;
    uint32_t dispCntSub;
    uint16_t bgCntMain[4];
    uint16_t bgCntSub[4];
    uint16_t blendCntMain;
    uint16_t blendCntSub;
    uint16_t blendAlpha;
    int16_t  masterBrightMain;
    int16_t  masterBrightSub;
    uint8_t  vramBuffer[NDS_VRAM_BUFFER_SIZE];
} HardwareIoRegisters;

/* Memory arenas */
extern uint8_t gHardwareIoRegMemory[NDS_IO_REG_SIZE];
extern uint8_t gHardwareMainRam[NDS_MAIN_RAM_SIZE];

/* Lifecycle & Accessors */
void PortHwRegisters_Init(void);
HardwareIoRegisters* PortHwRegisters_Get(void);
uint8_t* PortHwRegisters_GetIoRegBase(void);
uint8_t* PortHwRegisters_GetMainRamBase(void);

#ifdef __cplusplus
}
#endif

#endif // PORT_HW_REGISTERS_H
