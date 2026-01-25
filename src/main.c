#include "cart.h"
#include "cpu.h"
#include "nes.h"

#define NESTEST_INSTRUCTION_LIMIT 8991

int main(int argc, char* argv[]) {
    const char* rom_path = (argc > 1) ? argv[1] : "../test_roms/nestest.nes";

    NES* nes = nes_create();
    if (!nes) {
        printf("error: could not create NES\n");
        return 1;
    }

    if (!nes_load_cartridge(nes, rom_path)) {
        printf("error: could not load cartridge: %s\n", rom_path);
        nes_destroy(nes);
        return 1;
    }

    print_cart_info(nes->cartridge);
    // For nestest automation mode, start at $C000
    nes->cpu.regs.pc = 0xC000;

    for (int i = 0; i < NESTEST_INSTRUCTION_LIMIT; i++) {
        cpu_trace(&nes->cpu);
        cpu_step(&nes->cpu);
    }

    nes_destroy(nes);
    return 0;
}
