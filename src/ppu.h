#ifndef PPU_H
#define PPU_H

#include <stdbool.h>
#include <stdint.h>

#define PPU_PATTERN_TABLE_0 0x0000
#define PPU_PATTERN_TABLE_1 0x1000
#define PPU_NAMETABLE_0 0x2000
#define PPU_NAMETABLE_1 0x2400
#define PPU_NAMETABLE_2 0x2800
#define PPU_NAMETABLE_3 0x2C00
#define PPU_PALETTE_START 0x3F00

// Screen dimensions
#define NES_WIDTH 256
#define NES_HEIGHT 240

// Timing
#define SCANLINES_PER_FRAME 262
#define DOTS_PER_SCANLINE 341
#define VBLANK_SCANLINE 241
#define PRE_RENDER_SCANLINE 261

struct Cartridge;

typedef struct PPU {
    //  Memory mapped registers
    uint8_t ctrl;     // $2000 PPUCTRL (write)
    uint8_t mask;     // $2001 PPUMASK (write)
    uint8_t status;   // $2002 PPUSTATUS (read)
    uint8_t oam_addr; // $2003 OAMADDR (write)

    // Internal VRAM
    uint8_t nametable_ram[2048]; // 2KB nametable RAM
    uint8_t palette_ram[32];     // 32 bytes palette RAM
    uint8_t oam[256];            // 256 bytes OAM (sprite data)

    // Loopy registers (internal scroll/address)
    uint16_t v; // Current VRAM address (15 bits)
    uint16_t t; // Temporary VRAM address (15 bits)
    uint8_t x;  // Fine X scroll (3 bits)
    bool w;     // Write toggle (shared by PPUSCROLL and PPUADDR)

    // Internal read buffer for PPUDATA
    uint8_t data_buf;

    // Rendering state
    int scanline;
    int dot;
    bool nmi_occurred;
    bool nmi_output;
    bool frame_complete;

    // Frame buffer (NES palette indices)
    uint8_t framebuf[NES_WIDTH * NES_HEIGHT];

    // Cartridge reference for CHR-ROM
    struct Cartridge* cart;

} PPU;

void ppu_init(PPU* ppu);
void ppu_connect_cartridge(PPU* ppu, struct Cartridge* cart);

// Register access (called via bus for $2000-$2007)
uint8_t ppu_read_register(PPU* ppu, uint16_t addr);
void ppu_write_register(PPU* ppu, uint16_t addr, uint8_t data);

// PPU-internal VRAM access
uint8_t ppu_read_vram(PPU* ppu, uint16_t addr);
void ppu_write_vram(PPU* ppu, uint16_t addr, uint8_t data);

void ppu_step(PPU* ppu);

bool ppu_nmi_pending(PPU* ppu);
void ppu_clear_nmi(PPU* ppu);

#endif
