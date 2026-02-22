#ifndef NES_H
#define NES_H

#include "apu.h"
#include "bus.h"
#include "cart.h"
#include "cpu.h"
#include "ppu.h"

typedef struct NES {
    CPU_6502 cpu;
    Bus bus;
    PPU ppu;
    APU apu;
    Cartridge* cartridge;
} NES;

NES* nes_create();
void nes_destroy(NES* nes);
int nes_load_cartridge(NES* nes, const char* path);
void nes_reset(NES* nes);
void nes_step(NES* nes);

#endif
