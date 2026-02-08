#include "bus.h"
#include "cart.h"
#include "ppu.h"
#include <stdio.h>
#include <string.h>

void bus_init(Bus* bus) { memset(bus, 0, sizeof(Bus)); }

void bus_connect_cartridge(Bus* bus, Cartridge* cartridge) {
    bus->cartridge = cartridge;
}

void bus_write_byte(Bus* bus, uint16_t addr, uint8_t data) {
    if (addr <= RAM_MIRROR_END) {
        bus->ram[RAM_MIRROR_TO_BASE(addr)] = data;
    }

    else if (addr <= PPU_MIRROR_END) {
        ppu_write_register(bus->ppu, PPU_MIRROR_TO_BASE(addr), data);
    }

    else if (addr == 0x4014) {
        // OAM DMA: copy 256 bytes from CPU page to OAM
        uint16_t page = (uint16_t)data << 8;
        for (int i = 0; i < 256; i++) {
            bus->ppu->oam[(bus->ppu->oam_addr + i) & 0xFF] =
                bus_read_byte(bus, page + i);
        }
    }

    else if (addr <= IO_END) {
        printf("unimplemented: write to IO\n");
    }

    else if (addr <= EXPANSION_END) {
        printf("unimplemented: write to expansion\n");
    }

    else if (addr <= SRAM_END) {
        printf("unimplemented: write to SRAM\n");
    }

    else {
        // PRG-ROM ($8000-$FFFF)
        printf("unimplemented: write to cartridge\n");
    }
}

uint8_t bus_read_byte(Bus* bus, uint16_t addr) {
    uint8_t data = 0;

    if (addr <= RAM_MIRROR_END) {
        data = bus->ram[RAM_MIRROR_TO_BASE(addr)];
    }

    else if (addr <= PPU_MIRROR_END) {
        data = ppu_read_register(bus->ppu, PPU_MIRROR_TO_BASE(addr));
    }

    else if (addr <= IO_END) {
        printf("unimplemented: read from IO\n");
    }

    else if (addr <= EXPANSION_END) {
        printf("unimplemented: read from expansion\n");
    }

    else if (addr <= SRAM_END) {
        printf("unimplemented: read from SRAM\n");
    }

    else {
        // PRG-ROM ($8000-$FFFF)
        data = cart_read_byte(bus->cartridge, addr);
    }

    return data;
}

void bus_write_word(Bus* bus, uint16_t addr, uint16_t data) {
    bus_write_byte(bus, addr, data & 0xFF);
    bus_write_byte(bus, addr + 1, data >> 8);
}

uint16_t bus_read_word(Bus* bus, uint16_t addr) {
    return bus_read_byte(bus, addr) | (bus_read_byte(bus, addr + 1) << 8);
}