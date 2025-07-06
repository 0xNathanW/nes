#ifndef MEM_MAP_H
#define MEM_MAP_H

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

// Region checking.
#define IS_RAM_ADDR(addr)       ((addr) <= RAM_MIRROR_END)
#define IS_PPU_ADDR(addr)       ((addr) >= PPU_START && (addr) <= PPU_MIRROR_END)
#define IS_IO_ADDR(addr)        ((addr) >= IO_START && (addr) <= IO_END)
#define IS_CART_ROM_ADDR(addr)  ((addr) >= EXPANSION_START && (addr) <= PRG_ROM_END)
#define IS_EXPANSION_ADDR(addr) ((addr) >= EXPANSION_START && (addr) <= EXPANSION_END)
#define IS_SRAM_ADDR(addr)      ((addr) >= SRAM_START && (addr) <= SRAM_END)
#define IS_PRG_ROM_ADDR(addr)   ((addr) >= PRG_ROM_START && (addr) <= PRG_ROM_END)

// Mirror address conversion.
#define RAM_MIRROR_TO_BASE(addr)  ((addr) & 0x07FF)
#define PPU_MIRROR_TO_BASE(addr)  (PPU_START + ((addr) & 0x0007))

#endif