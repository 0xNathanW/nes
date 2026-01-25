#include "nes.h"
#include "bus.h"
#include "cpu.h"
#include <stdlib.h>
#include <string.h>

NES* nes_create() {
    
    NES* nes = (NES*)malloc(sizeof(NES));
    if (!nes) {
        return NULL;
    }
    memset(nes, 0, sizeof(NES));

    cpu_init(&nes->cpu);
    bus_init(&nes->bus);

    // Connect the bus to components.
    nes->bus.cpu = &nes->cpu;
    nes->bus.cartridge = nes->cartridge;

    nes->cpu.bus = &nes->bus;
    nes->bus.cpu = &nes->cpu;

    return nes;
}

void nes_destroy(NES* nes) {
    if (nes) {
        if (nes->cartridge) {
            free(nes->cartridge);
        }
        free(nes);
    }
}

int nes_load_cartridge(NES* nes, const char* path) {
    nes->cartridge = load_cart(path);
    if (!nes->cartridge) {
        return 0;
    }

    nes->bus.cartridge = nes->cartridge;
    cpu_power_on(&nes->cpu);
    return 1;
}

void nes_reset(NES* nes) {
    cpu_reset(&nes->cpu);
}

void nes_step(NES* nes) {
    uint8_t opcode = bus_read_byte(&nes->bus, nes->cpu.regs.pc++);
    printf("opcode: %02X\n", opcode);
    
}