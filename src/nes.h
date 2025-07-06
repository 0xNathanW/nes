#ifndef NES_H
#define NES_H

#include "cpu.h"
#include "ines.h"
#include "bus.h"

typedef struct NES {
    CPU_6502 cpu;
    Bus bus;
    INES_Cart* cartridge;
} NES;

NES* nes_create();
void nes_destroy(NES* nes);
int nes_load_cartridge(NES* nes, const char* path);
void nes_step(NES* nes);

#endif
