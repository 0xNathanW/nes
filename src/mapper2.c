#include "cart.h"
#include "log.h"
#include <stdlib.h>

typedef struct {
    uint8_t prg_bank;
} UxROMState;

static uint8_t mapper2_read_prg(Cartridge* cart, uint16_t addr) {
    if (addr < 0x8000)
        return 0;

    UxROMState* state = (UxROMState*)cart->mapper_data;
    uint32_t prg_size = cart->prg_rom_size * PRG_BLOCK;
    uint32_t offset;

    if (addr < 0xC000)
        offset = (uint32_t)state->prg_bank * PRG_BLOCK + (addr - 0x8000);
    else
        offset = (uint32_t)(cart->prg_rom_size - 1) * PRG_BLOCK +
                 (addr - 0xC000);

    return cart->prg_rom[offset % prg_size];
}

static void mapper2_write_prg(Cartridge* cart, uint16_t addr, uint8_t data) {
    if (addr < 0x8000)
        return;

    UxROMState* state = (UxROMState*)cart->mapper_data;
    state->prg_bank = data & 0x0F;
}

static uint8_t mapper2_read_chr(Cartridge* cart, uint16_t addr) {
    if (addr >= 0x2000)
        return 0;

    if (cart->chr_rom_size > 0)
        return cart->chr_rom[addr];
    return cart->chr_ram[addr];
}

static void mapper2_write_chr(Cartridge* cart, uint16_t addr, uint8_t data) {
    if (addr < 0x2000 && cart->chr_rom_size == 0)
        cart->chr_ram[addr] = data;
}

void mapper2_init(Cartridge* cart) {
    UxROMState* state = malloc(sizeof(UxROMState));
    if (!state) {
        LOG_ERROR("unable to allocate memory for mapper 2 (UxROM) state");
        return;
    }
    state->prg_bank = 0;

    cart->mapper_data = state;
    cart->mapper.read_prg = mapper2_read_prg;
    cart->mapper.write_prg = mapper2_write_prg;
    cart->mapper.read_chr = mapper2_read_chr;
    cart->mapper.write_chr = mapper2_write_chr;
}
