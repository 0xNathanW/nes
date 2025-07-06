#include "nes.h"
#include "ines.h"
#include "cpu.h"
#include <stdlib.h>
#include "mem_map.h"

NES* create_nes(INES_Cart* cart) {
    NES* nes = (NES*)malloc(sizeof(NES));
    if (!nes) {
        return NULL;
    }
    
    reset_cpu(&nes->cpu);
    nes->cart = cart;
    return nes;
}

void destroy_nes(NES* nes) {
    if (nes) {
        free(nes);
    }
}

uint8_t bus_read_byte(NES* nes, uint16_t addr) {
    if (IS_RAM_ADDR(addr)) {
        // Mirror every 2KB.
        return nes->cpu.ram[RAM_MIRROR_TO_BASE(addr)];
    }

    else if (IS_PPU_ADDR(addr)) {
        // TODO: implement.
        return 0;
    }

    else if (IS_IO_ADDR(addr)) {
        // TODO: implement.
        return 0;
    }

    else if (IS_EXPANSION_ADDR(addr)) {
        // TODO: implement.
        return 0;
    }

    else if (IS_SRAM_ADDR(addr)) {
        // TODO: implement.
        return 0;
    }

    return 0;
}

void bus_write_byte(NES* nes, uint16_t addr, uint8_t data) {
    if (IS_RAM_ADDR(addr)) {
        nes->cpu.ram[RAM_MIRROR_TO_BASE(addr)] = data;
    }

    else if (IS_PPU_ADDR(addr)) {
        // TODO: implement.
    }
    
}
