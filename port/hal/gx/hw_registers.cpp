/**
 * @file hw_registers.cpp
 * @brief Emulated Nintendo DS I/O registers and memory arenas.
 */

#include "port_hw_registers.h"
#include <cstring>

alignas(64) uint8_t gHardwareIoRegMemory[NDS_IO_REG_SIZE] = {0};
alignas(64) uint8_t gHardwareMainRam[NDS_MAIN_RAM_SIZE] = {0};

static HardwareIoRegisters s_hardwareIoRegisters;

void PortHwRegisters_Init(void) {
    std::memset(gHardwareIoRegMemory, 0, sizeof(gHardwareIoRegMemory));
    std::memset(gHardwareMainRam, 0, sizeof(gHardwareMainRam));
    std::memset(&s_hardwareIoRegisters, 0, sizeof(s_hardwareIoRegisters));
}

HardwareIoRegisters* PortHwRegisters_Get(void) {
    return &s_hardwareIoRegisters;
}

uint8_t* PortHwRegisters_GetIoRegBase(void) {
    return gHardwareIoRegMemory;
}

uint8_t* PortHwRegisters_GetMainRamBase(void) {
    return gHardwareMainRam;
}
