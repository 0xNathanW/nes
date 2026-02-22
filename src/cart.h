#ifndef CART_H
#define CART_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "mapper.h"

// https://www.nesdev.org/wiki/INES

/*
TODO: do we need to keep header in the cart?
*/

// 16KB
#define PRG_BLOCK 16384
// 8KB
#define CHR_BLOCK 8192

// 16-byte header for iNES roms.
// General rule, if last 4 bytes are not all set to zero, and the ines 2.0 is
// not used, refuse to load the ROM.
struct CartHeader {
    // Constant $4E $45 $53 $1A (ASCII "NES" followed by MS-DOS end-of-file).
    char signature[4];
    // Size of PRG ROM (program code) in 16 KB units, generally 32KB.
    uint8_t prg_rom_size;
    // Size of CHR ROM (graphics) in 8 KB units (value 0 means the board uses CHR RAM).
    uint8_t chr_rom_size;
    /*
    76543210
    ||||||||
    |||||||+- Nametable arrangement: 0: horizontal mirroring (CIRAM A10 = PPU A11)
    |||||||                          1: vertical mirroring   (CIRAM A10 = PPU A10)
    ||||||+-- 1: Cartridge contains battery-backed PRG RAM ($6000-7FFF) or other persistent memory
    |||||+--- 1: 512-byte trainer at $7000-$71FF (stored before PRG data)
    ||||+---- 1: Alternative nametable layout
    ++++----- Lower nybble of mapper number
    */
    uint8_t flags6;
    /*
    76543210
    ||||||||
    |||||||+- VS Unisystem
    ||||||+-- PlayChoice-10 (8 KB of Hint Screen data stored after CHR data)
    ||||++--- If equal to 2, flags 8-15 are in NES 2.0 format
    ++++----- Upper nybble of mapper number
    */
    uint8_t flags7;
    /*
    76543210
    ||||||||
    ++++++++- PRG RAM size
    */
    uint8_t flags8;
    /*
    76543210
    ||||||||
    |||||||+- TV system (0: NTSC; 1: PAL)
    +++++++-- Reserved, set to zero
    */
    uint8_t flags9;
    /*
    76543210
      ||  ||
      ||  ++- TV system (0: NTSC; 2: PAL; 1/3: dual compatible) (unused)
      |+----- PRG RAM ($6000-$7FFF) (0: present; 1: not present) (virtually unused)
      +------ 0: Board has no bus conflicts; 1: Board has bus conflicts
    */
    uint8_t flags10;

    // 5 bytes unused padding (should be filled with zeros).
    char padding[5];
};

typedef struct Cartridge {
    struct CartHeader header;
    // The trainer usually contains mapper register translation and CHR-RAM caching code for
    // - early RAM cartridges that could not mimic mapper ASICs and only had 32 KiB of CHR-RAM;
    // - Nesticle, an old but influential NES emulator for DOS.
    // It is not used on unmodified dumps of original ROM cartridges.
    bool has_trainer;
    uint8_t* trainer;
    // Part of ROM that contains game code (instructions), size defined in header.
    uint8_t prg_rom_size;
    uint8_t* prg_rom;
    // Part of ROM that contains graphics data, size defined in header.
    uint8_t chr_rom_size;
    uint8_t* chr_rom;
    // 0 = horizontal mirroring, 1 = vertical mirroring.
    uint8_t nametable_arrangement;
    // Persistant memory is usualy in the form of battery backed prg-ram at
    // 0x6000, but there mapper specific exceptions.
    bool is_battery_backed;
    uint8_t mapper_number;

    // Mapper vtable and per-mapper state
    Mapper mapper;
    void* mapper_data;
    bool irq_pending;

    // CHR-RAM (used when chr_rom_size == 0)
    uint8_t chr_ram[0x2000];
} Cartridge;

Cartridge* load_cart(const char* path);
void free_cart(Cartridge* cart);
void print_cart_info(Cartridge* cart);

uint8_t cart_read_byte(Cartridge* cart, uint16_t addr);
void cart_write_byte(Cartridge* cart, uint16_t addr, uint8_t data);

// PPU-side CHR access ($0000-$1FFF in PPU address space)
uint8_t cart_read_chr(Cartridge* cart, uint16_t addr);
void cart_write_chr(Cartridge* cart, uint16_t addr, uint8_t data);

#endif
