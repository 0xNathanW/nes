#include "nes.h"
#include "bus.h"
#include "cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

NES* nes_create() {
    NES* nes = (NES*)malloc(sizeof(NES));
    if (!nes) {
        return NULL;
    }
    memset(nes, 0, sizeof(NES));

    bus_init(&nes->bus);
    nes->cpu.bus = &nes->bus;

    return nes;
}

void nes_destroy(NES* nes) {
    if (nes) {
        if (nes->cartridge) {
            free_cart(nes->cartridge);
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

void nes_reset(NES* nes) { cpu_reset(&nes->cpu); }
