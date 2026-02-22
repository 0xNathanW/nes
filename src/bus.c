#include "bus.h"
#include "apu.h"
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

    else if (addr == 0x4016) {
        bool new_strobe = (data & 1) != 0;
        if (bus->controller_strobe && !new_strobe) {
            // Falling edge: latch controller state into shift registers
            bus->controller_shift[0] = bus->controller_state[0];
            bus->controller_shift[1] = bus->controller_state[1];
        }
        bus->controller_strobe = new_strobe;
    }

    else if (addr <= IO_END) {
        apu_write_register(bus->apu, addr, data);
    }

    else if (addr <= EXPANSION_END) {
        // Expansion ROM — ignored
    }

    else if (addr <= SRAM_END) {
        bus->sram[addr - SRAM_START] = data;
    }

    else {
        // PRG-ROM ($8000-$FFFF) — forward to mapper
        cart_write_byte(bus->cartridge, addr, data);
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

    else if (addr == 0x4015) {
        data = apu_read_register(bus->apu, addr);
    }

    else if (addr == 0x4016 || addr == 0x4017) {
        int port = addr & 1;
        if (bus->controller_strobe) {
            // While strobe is high, always return current state of A button
            data = bus->controller_state[port] & 1;
        } else {
            // Shift out one button bit per read
            data = bus->controller_shift[port] & 1;
            bus->controller_shift[port] >>= 1;
        }
    }

    else if (addr <= IO_END) {
        // APU and I/O — returns 0 for now
    }

    else if (addr <= EXPANSION_END) {
        // Expansion ROM — returns 0
    }

    else if (addr <= SRAM_END) {
        data = bus->sram[addr - SRAM_START];
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