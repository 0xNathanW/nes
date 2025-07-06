#include "bus.h"
#include "ines.h"
#include "cpu.h"
#include <string.h>
#include <stdio.h>

void bus_init(Bus* bus) {
    memset(bus, 0, sizeof(Bus));
}

void bus_connect_cartridge(Bus* bus, INES_Cart* cartridge) {
    bus->cartridge = cartridge;
}

void bus_write_byte(Bus* bus, uint16_t addr, uint8_t data) {
    if (addr <= RAM_MIRROR_END) {
        bus->cpu->ram[RAM_MIRROR_TO_BASE(addr)] = data;
    }

    else if (addr <= PPU_MIRROR_END) {
        printf("unimplemented: write to PPU\n");
    }

    else if (addr <= IO_END) {
        printf("unimplemented: write to IO\n");
    }

    else if (addr <= PRG_ROM_END) {
        printf("unimplemented: write to cartridge\n");
    }

    else if (addr <= EXPANSION_END) {
        printf("unimplemented: write to expansion\n");
    }

    else if (addr <= SRAM_END) {
        printf("unimplemented: write to SRAM\n");
    }
}

uint8_t bus_read_byte(Bus* bus, uint16_t addr) {
    uint8_t data = 0;
    
    if (addr <= RAM_MIRROR_END) {
        data = bus->cpu->ram[RAM_MIRROR_TO_BASE(addr)];
    }

    else if (addr <= PPU_MIRROR_END) {
        printf("unimplemented: read from PPU\n");
    }

    else if (addr <= IO_END) {
        printf("unimplemented: read from IO\n");
    }

    else if (addr <= PRG_ROM_END) {
        printf("unimplemented: read from cartridge\n");
    }

    else if (addr <= EXPANSION_END) {
        printf("unimplemented: read from expansion\n");
    }

    else if (addr <= SRAM_END) {
        printf("unimplemented: read from SRAM\n");
    }

    return data;
}

void bus_write_word(Bus* bus, uint16_t addr, uint16_t data) {
    bus_write_byte(bus, addr, data & 0xFF);
    bus_write_byte(bus, addr + 1, data >> 8);
}

uint16_t bus_read_word(Bus* bus, uint16_t addr) {
    return (bus_read_byte(bus, addr) << 8) | bus_read_byte(bus, addr + 1);
}