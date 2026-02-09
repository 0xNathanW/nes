#include "cart.h"

static uint8_t mapper0_read_prg(Cartridge* cart, uint16_t addr) {
    if (addr >= 0x8000) {
        uint16_t offset = addr - 0x8000;
        if (cart->prg_rom_size == 1) {
            offset &= 0x3FFF;
        }
        return cart->prg_rom[offset];
    }
    return 0;
}

static void mapper0_write_prg(Cartridge* cart, uint16_t addr, uint8_t data) {
    (void)cart;
    (void)addr;
    (void)data;
}

static uint8_t mapper0_read_chr(Cartridge* cart, uint16_t addr) {
    if (addr < 0x2000) {
        if (cart->chr_rom_size > 0) {
            return cart->chr_rom[addr];
        }
        return cart->chr_ram[addr];
    }
    return 0;
}

static void mapper0_write_chr(Cartridge* cart, uint16_t addr, uint8_t data) {
    if (addr < 0x2000 && cart->chr_rom_size == 0) {
        cart->chr_ram[addr] = data;
    }
}

void mapper0_init(Cartridge* cart) {
    cart->mapper.read_prg = mapper0_read_prg;
    cart->mapper.write_prg = mapper0_write_prg;
    cart->mapper.read_chr = mapper0_read_chr;
    cart->mapper.write_chr = mapper0_write_chr;
    cart->mapper_data = NULL;
}
