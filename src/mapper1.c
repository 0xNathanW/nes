#include "cart.h"
#include <stdlib.h>

typedef struct {
    uint8_t shift_register;
    uint8_t shift_count;
    uint8_t reg_control;   // Register 0: mirroring, PRG mode, CHR mode
    uint8_t reg_chr_bank0; // Register 1: CHR bank for $0000-$0FFF
    uint8_t reg_chr_bank1; // Register 2: CHR bank for $1000-$1FFF
    uint8_t reg_prg_bank;  // Register 3: PRG bank + RAM enable
} MMC1State;

static void mmc1_update_mirroring(Cartridge* cart, MMC1State* state) {
    switch (state->reg_control & 0x03) {
    case 0:
        cart->nametable_arrangement = MIRROR_ONESCREEN_LO;
        break;
    case 1:
        cart->nametable_arrangement = MIRROR_ONESCREEN_HI;
        break;
    case 2:
        cart->nametable_arrangement = MIRROR_VERTICAL;
        break;
    case 3:
        cart->nametable_arrangement = MIRROR_HORIZONTAL;
        break;
    }
}

static uint8_t mapper1_read_prg(Cartridge* cart, uint16_t addr) {
    if (addr < 0x8000)
        return 0;

    MMC1State* state = (MMC1State*)cart->mapper_data;
    uint8_t prg_mode = (state->reg_control >> 2) & 0x03;
    uint32_t prg_size = cart->prg_rom_size * PRG_BLOCK;
    uint32_t offset;

    switch (prg_mode) {
    case 0:
    case 1:
        offset = (uint32_t)(state->reg_prg_bank & 0x0E) * PRG_BLOCK +
                 (addr - 0x8000);
        break;
    case 2:
        if (addr < 0xC000)
            offset = addr - 0x8000;
        else
            offset = (uint32_t)(state->reg_prg_bank & 0x0F) * PRG_BLOCK +
                     (addr - 0xC000);
        break;
    case 3:
    default:
        if (addr < 0xC000)
            offset = (uint32_t)(state->reg_prg_bank & 0x0F) * PRG_BLOCK +
                     (addr - 0x8000);
        else
            offset = (uint32_t)(cart->prg_rom_size - 1) * PRG_BLOCK +
                     (addr - 0xC000);
        break;
    }

    return cart->prg_rom[offset % prg_size];
}

static void mapper1_write_prg(Cartridge* cart, uint16_t addr, uint8_t data) {
    if (addr < 0x8000)
        return;

    MMC1State* state = (MMC1State*)cart->mapper_data;

    if (data & 0x80) {
        state->shift_register = 0;
        state->shift_count = 0;
        state->reg_control |= 0x0C;
        return;
    }

    state->shift_register |= (data & 0x01) << state->shift_count;
    state->shift_count++;

    if (state->shift_count < 5)
        return;

    uint8_t value = state->shift_register;
    uint8_t reg = (addr >> 13) & 0x03;

    switch (reg) {
    case 0:
        state->reg_control = value;
        mmc1_update_mirroring(cart, state);
        break;
    case 1:
        state->reg_chr_bank0 = value;
        break;
    case 2:
        state->reg_chr_bank1 = value;
        break;
    case 3:
        state->reg_prg_bank = value;
        break;
    }

    state->shift_register = 0;
    state->shift_count = 0;
}

static uint8_t mapper1_read_chr(Cartridge* cart, uint16_t addr) {
    if (addr >= 0x2000)
        return 0;

    MMC1State* state = (MMC1State*)cart->mapper_data;
    bool use_ram = (cart->chr_rom_size == 0);
    uint8_t* chr = use_ram ? cart->chr_ram : cart->chr_rom;
    uint32_t chr_size =
        use_ram ? 0x2000 : (uint32_t)cart->chr_rom_size * CHR_BLOCK;

    uint32_t offset;
    if (state->reg_control & 0x10) {
        // 4KB mode
        if (addr < 0x1000)
            offset = (uint32_t)state->reg_chr_bank0 * 0x1000 + addr;
        else
            offset = (uint32_t)state->reg_chr_bank1 * 0x1000 + (addr - 0x1000);
    } else {
        // 8KB mode: bank0 & 0x1E selects 8KB-aligned bank
        offset = (uint32_t)(state->reg_chr_bank0 & 0x1E) * 0x1000 + addr;
    }

    return chr[offset % chr_size];
}

static void mapper1_write_chr(Cartridge* cart, uint16_t addr, uint8_t data) {
    if (addr >= 0x2000 || cart->chr_rom_size != 0)
        return;

    MMC1State* state = (MMC1State*)cart->mapper_data;
    uint32_t offset;

    if (state->reg_control & 0x10) {
        if (addr < 0x1000)
            offset = (uint32_t)state->reg_chr_bank0 * 0x1000 + addr;
        else
            offset = (uint32_t)state->reg_chr_bank1 * 0x1000 + (addr - 0x1000);
    } else {
        offset = (uint32_t)(state->reg_chr_bank0 & 0x1E) * 0x1000 + addr;
    }

    cart->chr_ram[offset % 0x2000] = data;
}

void mapper1_init(Cartridge* cart) {
    MMC1State* state = malloc(sizeof(MMC1State));
    state->shift_register = 0;
    state->shift_count = 0;
    state->reg_control = 0x0C;
    state->reg_chr_bank0 = 0;
    state->reg_chr_bank1 = 0;
    state->reg_prg_bank = 0;

    cart->mapper_data = state;
    cart->mapper.read_prg = mapper1_read_prg;
    cart->mapper.write_prg = mapper1_write_prg;
    cart->mapper.read_chr = mapper1_read_chr;
    cart->mapper.write_chr = mapper1_write_chr;

    mmc1_update_mirroring(cart, state);
}
