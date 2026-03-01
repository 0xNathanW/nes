#include "cart.h"
#include "log.h"
#include <stdlib.h>

typedef struct {
    uint8_t bank_select;  // $8000: bank select register
    uint8_t bank_regs[8]; // R0-R7 bank registers
    uint8_t irq_latch;    // $C000: counter reload value
    uint8_t irq_counter;
    bool irq_reload;
    bool irq_enabled;
} MMC3State;

static uint8_t mapper4_read_prg(Cartridge* cart, uint16_t addr) {
    if (addr < 0x8000)
        return 0;

    MMC3State* state = (MMC3State*)cart->mapper_data;
    uint32_t prg_size = cart->prg_rom_size * PRG_BLOCK;
    uint8_t num_8k_banks = cart->prg_rom_size * 2;
    uint32_t offset;

    // Each PRG slot is 8KB ($2000 bytes)
    uint8_t r6 = state->bank_regs[6] % num_8k_banks;
    uint8_t r7 = state->bank_regs[7] % num_8k_banks;
    uint8_t second_last = (num_8k_banks - 2) % num_8k_banks;
    uint8_t last = (num_8k_banks - 1) % num_8k_banks;

    if (state->bank_select & 0x40) {
        // PRG mode 1
        if (addr < 0xA000)
            offset = (uint32_t)second_last * 0x2000 + (addr - 0x8000);
        else if (addr < 0xC000)
            offset = (uint32_t)r7 * 0x2000 + (addr - 0xA000);
        else if (addr < 0xE000)
            offset = (uint32_t)r6 * 0x2000 + (addr - 0xC000);
        else
            offset = (uint32_t)last * 0x2000 + (addr - 0xE000);
    } else {
        // PRG mode 0
        if (addr < 0xA000)
            offset = (uint32_t)r6 * 0x2000 + (addr - 0x8000);
        else if (addr < 0xC000)
            offset = (uint32_t)r7 * 0x2000 + (addr - 0xA000);
        else if (addr < 0xE000)
            offset = (uint32_t)second_last * 0x2000 + (addr - 0xC000);
        else
            offset = (uint32_t)last * 0x2000 + (addr - 0xE000);
    }

    return cart->prg_rom[offset % prg_size];
}

static void mapper4_write_prg(Cartridge* cart, uint16_t addr, uint8_t data) {
    if (addr < 0x8000)
        return;

    MMC3State* state = (MMC3State*)cart->mapper_data;
    bool even = (addr & 1) == 0;

    if (addr < 0xA000) {
        if (even) {
            state->bank_select = data;
        } else {
            uint8_t reg = state->bank_select & 0x07;
            state->bank_regs[reg] = data;
        }
    } else if (addr < 0xC000) {
        if (even) {
            cart->nametable_arrangement =
                (data & 0x01) ? MIRROR_HORIZONTAL : MIRROR_VERTICAL;
        }
    } else if (addr < 0xE000) {
        if (even) {
            state->irq_latch = data;
        } else {
            state->irq_reload = true;
        }
    } else {
        if (even) {
            state->irq_enabled = false;
            cart->irq_pending = false;
        } else {
            state->irq_enabled = true;
        }
    }
}

static uint8_t mapper4_read_chr(Cartridge* cart, uint16_t addr) {
    if (addr >= 0x2000)
        return 0;

    MMC3State* state = (MMC3State*)cart->mapper_data;
    bool use_ram = (cart->chr_rom_size == 0);
    uint8_t* chr = use_ram ? cart->chr_ram : cart->chr_rom;
    uint32_t chr_size =
        use_ram ? 0x2000 : (uint32_t)cart->chr_rom_size * CHR_BLOCK;

    uint32_t offset;
    bool inversion = (state->bank_select & 0x80) != 0;

    if (!inversion) {
        if (addr < 0x0800)
            offset = (uint32_t)(state->bank_regs[0] & 0xFE) * 0x0400 + addr;
        else if (addr < 0x1000)
            offset = (uint32_t)(state->bank_regs[1] & 0xFE) * 0x0400 +
                     (addr - 0x0800);
        else if (addr < 0x1400)
            offset = (uint32_t)state->bank_regs[2] * 0x0400 + (addr - 0x1000);
        else if (addr < 0x1800)
            offset = (uint32_t)state->bank_regs[3] * 0x0400 + (addr - 0x1400);
        else if (addr < 0x1C00)
            offset = (uint32_t)state->bank_regs[4] * 0x0400 + (addr - 0x1800);
        else
            offset = (uint32_t)state->bank_regs[5] * 0x0400 + (addr - 0x1C00);
    } else {
        if (addr < 0x0400)
            offset = (uint32_t)state->bank_regs[2] * 0x0400 + addr;
        else if (addr < 0x0800)
            offset = (uint32_t)state->bank_regs[3] * 0x0400 + (addr - 0x0400);
        else if (addr < 0x0C00)
            offset = (uint32_t)state->bank_regs[4] * 0x0400 + (addr - 0x0800);
        else if (addr < 0x1000)
            offset = (uint32_t)state->bank_regs[5] * 0x0400 + (addr - 0x0C00);
        else if (addr < 0x1800)
            offset = (uint32_t)(state->bank_regs[0] & 0xFE) * 0x0400 +
                     (addr - 0x1000);
        else
            offset = (uint32_t)(state->bank_regs[1] & 0xFE) * 0x0400 +
                     (addr - 0x1800);
    }

    return chr[offset % chr_size];
}

static void mapper4_write_chr(Cartridge* cart, uint16_t addr, uint8_t data) {
    if (addr >= 0x2000 || cart->chr_rom_size != 0)
        return;

    cart->chr_ram[addr] = data;
}

static void mapper4_scanline_counter(Cartridge* cart) {
    MMC3State* state = (MMC3State*)cart->mapper_data;

    if (state->irq_counter == 0 || state->irq_reload) {
        state->irq_counter = state->irq_latch;
        state->irq_reload = false;
    } else {
        state->irq_counter--;
    }

    if (state->irq_counter == 0 && state->irq_enabled) {
        cart->irq_pending = true;
    }
}

void mapper4_init(Cartridge* cart) {
    MMC3State* state = malloc(sizeof(MMC3State));
    if (!state) {
        LOG_ERROR("unable to allocate memory for mapper 4 (MMC3) state");
        return;
    }
    state->bank_select = 0;
    for (int i = 0; i < 8; i++)
        state->bank_regs[i] = 0;
    state->irq_latch = 0;
    state->irq_counter = 0;
    state->irq_reload = false;
    state->irq_enabled = false;

    cart->mapper_data = state;
    cart->mapper.read_prg = mapper4_read_prg;
    cart->mapper.write_prg = mapper4_write_prg;
    cart->mapper.read_chr = mapper4_read_chr;
    cart->mapper.write_chr = mapper4_write_chr;
    cart->mapper.scanline_counter = mapper4_scanline_counter;
}
