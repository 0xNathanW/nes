#ifndef BUS_H
#define BUS_H

#include <stdint.h>

/*
NES Memory Map:
$0000-$07FF: 2KB Internal RAM
$0800-$1FFF: Mirrors of $0000-$07FF (repeats 3 times)
$2000-$2007: PPU Control Registers
$2008-$3FFF: Mirrors of $2000-$2007 (repeats every 8 bytes)
$4000-$401F: APU and I/O Registers
$4020-$5FFF: Cartridge Expansion ROM
$6000-$7FFF: SRAM (Save RAM)
$8000-$FFFF: PRG-ROM (32KB)
*/

/********************************************************
_____________	0x10000

PRG-ROM

_____________	0x8000
SRAM
_____________	0x6000
Expansion ROM
_____________	0x4020


IO Registers


_____________	0x2000

RAM
_____________	0x0000
*********************************************************/

#define RAM_START         0x0000
#define RAM_END           0x07FF
#define RAM_SIZE          0x0800
#define RAM_MIRROR_START  0x0800
#define RAM_MIRROR_END    0x1FFF

#define PPU_START         0x2000
#define PPU_END           0x2007
#define PPU_SIZE          0x0008
#define PPU_MIRROR_START  0x2008
#define PPU_MIRROR_END    0x3FFF

#define IO_START          0x4000
#define IO_END            0x401F
#define IO_SIZE           0x0020

#define EXPANSION_START   0x4020
#define EXPANSION_END     0x5FFF
#define EXPANSION_SIZE    0x1FE0

#define SRAM_START        0x6000
#define SRAM_END          0x7FFF
#define SRAM_SIZE         0x2000

#define PRG_ROM_START     0x8000
#define PRG_ROM_END       0xFFFF
#define PRG_ROM_SIZE      0x8000

// Mirror address conversion.
#define RAM_MIRROR_TO_BASE(addr)  ((addr) & 0x07FF)
#define PPU_MIRROR_TO_BASE(addr)  (PPU_START + ((addr) & 0x0007))

struct CPU_6502;
struct Cartridge;

typedef struct Bus {
    struct Cartridge* cartridge;
    struct CPU_6502* cpu;
} Bus;

void bus_init(Bus* bus);
void bus_connect_cartridge(Bus* bus, struct Cartridge* cartridge);

void bus_write_byte(Bus* bus, uint16_t addr, uint8_t data);
uint8_t bus_read_byte(Bus* bus, uint16_t addr);

void bus_write_word(Bus* bus, uint16_t addr, uint16_t data);
uint16_t bus_read_word(Bus* bus, uint16_t addr);

#endif