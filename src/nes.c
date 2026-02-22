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
    ppu_init(&nes->ppu);
    apu_init(&nes->apu);
    nes->cpu.bus = &nes->bus;
    nes->bus.ppu = &nes->ppu;
    nes->bus.apu = &nes->apu;

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
    ppu_connect_cartridge(&nes->ppu, nes->cartridge);
    cpu_power_on(&nes->cpu);
    return 1;
}

void nes_reset(NES* nes) { cpu_reset(&nes->cpu); }

void nes_step(NES* nes) {
    int cpu_cycles = cpu_step(&nes->cpu);

    // PPU runs 3 dots per CPU cycle
    for (int i = 0; i < cpu_cycles * 3; i++) {
        ppu_step(&nes->ppu);
    }

    for (int i = 0; i < cpu_cycles; i++) {
        apu_step(&nes->apu);
    }

    // Check if PPU wants to trigger an NMI
    if (ppu_nmi_pending(&nes->ppu)) {
        nes->cpu.nmi_pending = true;
        ppu_clear_nmi(&nes->ppu);
    }

    if (nes->apu.irq_pending) {
        nes->cpu.irq_pending = true;
    }
}
