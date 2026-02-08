#include "ppu.h"
#include "cart.h"
#include "mapper.h"
#include <string.h>

void ppu_init(PPU* ppu) { memset(ppu, 0, sizeof(PPU)); }

void ppu_connect_cartridge(PPU* ppu, Cartridge* cart) { ppu->cart = cart; }

static uint16_t mirror_nametable_addr(PPU* ppu, uint16_t addr) {
    addr &= 0x0FFF;
    uint8_t mode = ppu->cart->nametable_arrangement;
    switch (mode) {
    case MIRROR_VERTICAL:
        return addr & 0x07FF;
    case MIRROR_HORIZONTAL:
        if (addr < 0x0800) {
            return addr & 0x03FF;
        } else {
            return 0x0400 + (addr & 0x03FF);
        }
    case MIRROR_ONESCREEN_LO:
        return addr & 0x03FF;
    case MIRROR_ONESCREEN_HI:
        return 0x0400 + (addr & 0x03FF);
    default:
        return addr & 0x07FF;
    }
}

uint8_t ppu_read_vram(PPU* ppu, uint16_t addr) {
    addr &= 0x3FFF;
    if (addr < 0x2000) {
        // Pattern tables
        return cart_read_chr(ppu->cart, addr);
    } else if (addr < 0x3F00) {
        // Nametables (with mirror of $3000-$3EFF -> $2000-$2EFF)
        return ppu->nametable_ram[mirror_nametable_addr(ppu, addr)];
    } else {
        // Palette RAM ($3F00-$3FFF)
        uint16_t pal_addr = addr & 0x1F;
        // Backdrop mirrors: $3F10, $3F14, $3F18, $3F1C -> $3F00, $3F04, $3F08, $3F0C
        if (pal_addr >= 0x10 && (pal_addr & 0x03) == 0) {
            pal_addr -= 0x10;
        }
        return ppu->palette_ram[pal_addr];
    }
}

void ppu_write_vram(PPU* ppu, uint16_t addr, uint8_t data) {
    addr &= 0x3FFF;
    if (addr < 0x2000) {
        cart_write_chr(ppu->cart, addr, data);
    } else if (addr < 0x3F00) {
        // Nametables
        ppu->nametable_ram[mirror_nametable_addr(ppu, addr)] = data;
    } else {
        // Palette RAM
        uint16_t pal_addr = addr & 0x1F;
        if (pal_addr >= 0x10 && (pal_addr & 0x03) == 0) {
            pal_addr -= 0x10;
        }
        ppu->palette_ram[pal_addr] = data;
    }
}

static void ppu_increment_v(PPU* ppu) { ppu->v += (ppu->ctrl & 0x04) ? 32 : 1; }

uint8_t ppu_read_register(PPU* ppu, uint16_t addr) {
    uint8_t data = 0;

    switch (addr) {
    // PPUSTATUS
    case 0x2002:
        data = (ppu->status & 0xE0) | (ppu->data_buf & 0x1F);
        ppu->status &= ~0x80; // Clear vblank flag
        ppu->nmi_occurred = false;
        ppu->w = false; // Reset write toggle
        break;

    // OAMDATA
    case 0x2004:
        data = ppu->oam[ppu->oam_addr];
        break;

    // PPUDATA
    case 0x2007:
        data = ppu->data_buf;
        ppu->data_buf = ppu_read_vram(ppu, ppu->v);
        // Palette reads are not buffered — return immediately
        if ((ppu->v & 0x3FFF) >= 0x3F00) {
            data = ppu->data_buf;
            // Buf gets the nametable byte "underneath" the palette
            ppu->data_buf =
                ppu->nametable_ram[mirror_nametable_addr(ppu, ppu->v)];
        }
        ppu_increment_v(ppu);
        break;

    default:
        // Others write-only.
        break;
    }

    return data;
}

void ppu_write_register(PPU* ppu, uint16_t addr, uint8_t data) {
    switch (addr) {

    // PPUCTRL
    case 0x2000:
        ppu->ctrl = data;
        ppu->nmi_output = (data & 0x80) != 0;
        // Nametable select bits go into t
        ppu->t = (ppu->t & 0xF3FF) | ((uint16_t)(data & 0x03) << 10);
        // If enabling NMI while in vblank, trigger NMI immediately
        if (ppu->nmi_output && ppu->nmi_occurred) {
            // NMI will be picked up by ppu_nmi_pending
        }
        break;

    // PPUMASK
    case 0x2001:
        ppu->mask = data;
        break;

    // OAMADDR
    case 0x2003:
        ppu->oam_addr = data;
        break;

    // OAMDATA
    case 0x2004:
        ppu->oam[ppu->oam_addr] = data;
        ppu->oam_addr++;
        break;

    // PPUSCROLL
    case 0x2005:
        if (!ppu->w) {
            // First write: X scroll
            ppu->t = (ppu->t & 0xFFE0) | (data >> 3);
            ppu->x = data & 0x07;
        } else {
            // Second write: Y scroll
            ppu->t = (ppu->t & 0x8C1F) | ((uint16_t)(data & 0x07) << 12) |
                     ((uint16_t)(data & 0xF8) << 2);
        }
        ppu->w = !ppu->w;
        break;

    // PPUADDR
    case 0x2006:
        if (!ppu->w) {
            // First write: high byte
            ppu->t = (ppu->t & 0x00FF) | ((uint16_t)(data & 0x3F) << 8);
        } else {
            // Second write: low byte, copy t to v
            ppu->t = (ppu->t & 0xFF00) | data;
            ppu->v = ppu->t;
        }
        ppu->w = !ppu->w;
        break;

    // PPUDATA
    case 0x2007:
        ppu_write_vram(ppu, ppu->v, data);
        ppu_increment_v(ppu);
        break;
    }
}

static void ppu_increment_scroll_x(PPU* ppu) {
    if ((ppu->v & 0x001F) == 31) {
        ppu->v &= ~0x001F;
        ppu->v ^= 0x0400; // Switch horizontal nametable
    } else {
        ppu->v++;
    }
}

static void ppu_increment_scroll_y(PPU* ppu) {
    if ((ppu->v & 0x7000) != 0x7000) {
        ppu->v += 0x1000;
    } else {
        ppu->v &= ~0x7000;
        int coarse_y = (ppu->v & 0x03E0) >> 5;
        if (coarse_y == 29) {
            coarse_y = 0;
            ppu->v ^= 0x0800; // Switch vertical nametable
        } else if (coarse_y == 31) {
            coarse_y = 0;
        } else {
            coarse_y++;
        }
        ppu->v = (ppu->v & ~0x03E0) | (coarse_y << 5);
    }
}

static void ppu_copy_horizontal_bits(PPU* ppu) {
    ppu->v = (ppu->v & ~0x041F) | (ppu->t & 0x041F);
}

static void ppu_copy_vertical_bits(PPU* ppu) {
    ppu->v = (ppu->v & ~0x7BE0) | (ppu->t & 0x7BE0);
}

// Renders one complete scanline combining background and sprite layers
static void ppu_render_scanline(PPU* ppu) {
    int sl = ppu->scanline;
    bool show_bg = (ppu->mask & 0x08) != 0;
    bool show_spr = (ppu->mask & 0x10) != 0;
    uint8_t backdrop = ppu_read_vram(ppu, 0x3F00) & 0x3F;

    uint8_t bg_cidx[NES_WIDTH];
    uint8_t bg_pixel[NES_WIDTH];
    memset(bg_cidx, 0, sizeof(bg_cidx));
    memset(bg_pixel, backdrop, sizeof(bg_pixel));

    if (show_bg) {
        uint16_t saved_v = ppu->v;
        ppu_copy_horizontal_bits(ppu);

        for (int px = 0; px < NES_WIDTH; px++) {
            uint16_t nt_addr = 0x2000 | (ppu->v & 0x0FFF);
            uint8_t tile_index = ppu_read_vram(ppu, nt_addr);

            uint16_t attr_addr = 0x23C0 | (ppu->v & 0x0C00) |
                                 ((ppu->v >> 4) & 0x38) |
                                 ((ppu->v >> 2) & 0x07);
            uint8_t attr_byte = ppu_read_vram(ppu, attr_addr);

            int coarse_x = ppu->v & 0x001F;
            int coarse_y = (ppu->v >> 5) & 0x001F;
            int shift = ((coarse_y & 2) << 1) | (coarse_x & 2);
            uint8_t palette_id = (attr_byte >> shift) & 0x03;

            uint16_t pattern_base = (ppu->ctrl & 0x10) ? 0x1000 : 0x0000;
            int fine_y = (ppu->v >> 12) & 0x07;
            uint8_t tile_lo =
                ppu_read_vram(ppu, pattern_base + tile_index * 16 + fine_y);
            uint8_t tile_hi =
                ppu_read_vram(ppu, pattern_base + tile_index * 16 + fine_y + 8);

            int fine_x_bit = 7 - ((px + ppu->x) & 7);
            uint8_t color_lo = (tile_lo >> fine_x_bit) & 1;
            uint8_t color_hi = (tile_hi >> fine_x_bit) & 1;
            uint8_t cidx = (color_hi << 1) | color_lo;

            bg_cidx[px] = cidx;
            if (cidx != 0) {
                bg_pixel[px] =
                    ppu_read_vram(ppu, 0x3F00 + palette_id * 4 + cidx) & 0x3F;
            }

            if (((px + ppu->x) & 7) == 7) {
                ppu_increment_scroll_x(ppu);
            }
        }

        ppu->v = (saved_v & 0x7BE0) | (ppu->v & ~0x7BE0);
    }

    // Scans OAM for up to 8 sprites on this scanline, renders back-to-front
    uint8_t spr_cidx[NES_WIDTH];
    uint8_t spr_pixel[NES_WIDTH];
    uint8_t spr_priority[NES_WIDTH];
    bool spr_zero[NES_WIDTH];
    memset(spr_cidx, 0, sizeof(spr_cidx));
    memset(spr_zero, 0, sizeof(spr_zero));

    if (show_spr) {
        bool tall = (ppu->ctrl & 0x20) != 0;
        int height = tall ? 16 : 8;
        uint16_t spr_table = (ppu->ctrl & 0x08) ? 0x1000 : 0x0000;

        // Sprite evaluation: find up to 8 in-range sprites
        int found = 0;
        int indices[8];

        for (int i = 0; i < 64; i++) {
            int diff = sl - (int)ppu->oam[i * 4];
            if (diff >= 0 && diff < height) {
                if (found < 8) {
                    indices[found++] = i;
                } else {
                    ppu->status |= 0x20; // Sprite overflow
                    break;
                }
            }
        }

        // Render back-to-front so lowest index (highest priority) writes last
        for (int s = found - 1; s >= 0; s--) {
            int i = indices[s];
            uint8_t spy = ppu->oam[i * 4];
            uint8_t tile = ppu->oam[i * 4 + 1];
            uint8_t attr = ppu->oam[i * 4 + 2];
            uint8_t spx = ppu->oam[i * 4 + 3];

            bool flip_h = (attr & 0x40) != 0;
            bool flip_v = (attr & 0x80) != 0;
            uint8_t pal = attr & 0x03;

            int row = sl - spy;
            if (flip_v)
                row = (height - 1) - row;

            uint16_t tile_addr;
            if (tall) {
                // 8x16: bit 0 of tile index selects pattern table
                uint16_t table = (tile & 1) ? 0x1000 : 0x0000;
                uint8_t tnum = tile & 0xFE;
                if (row >= 8) {
                    tnum++;
                    row -= 8;
                }
                tile_addr = table + tnum * 16 + row;
            } else {
                // 8x8: pattern table from PPUCTRL bit 3
                tile_addr = spr_table + tile * 16 + row;
            }

            uint8_t lo = ppu_read_vram(ppu, tile_addr);
            uint8_t hi = ppu_read_vram(ppu, tile_addr + 8);

            for (int p = 0; p < 8; p++) {
                int sx = spx + p;
                if (sx >= NES_WIDTH)
                    break;

                int bit = flip_h ? p : (7 - p);
                uint8_t cidx = (((hi >> bit) & 1) << 1) | ((lo >> bit) & 1);

                if (cidx == 0)
                    continue; // Transparent sprite pixel

                spr_cidx[sx] = cidx;
                spr_pixel[sx] =
                    ppu_read_vram(ppu, 0x3F10 + pal * 4 + cidx) & 0x3F;
                spr_priority[sx] = (attr >> 5) & 1;
                spr_zero[sx] = (i == 0);
            }
        }
    }

    // Combines BG and sprite outputs per-pixel per NES PPU spec
    for (int px = 0; px < NES_WIDTH; px++) {
        // Left-column clipping (PPUMASK bits 1-2)
        uint8_t bg = (px < 8 && !(ppu->mask & 0x02)) ? 0 : bg_cidx[px];
        uint8_t spr = (px < 8 && !(ppu->mask & 0x04)) ? 0 : spr_cidx[px];

        // Sprite 0 hit: both BG and sprite opaque, x != 255
        if (spr_zero[px] && bg != 0 && spr != 0 && px < 255) {
            ppu->status |= 0x40;
        }

        // Priority decision
        uint8_t output;
        if (bg == 0 && spr == 0) {
            output = backdrop;
        } else if (bg == 0) {
            output = spr_pixel[px];
        } else if (spr == 0) {
            output = bg_pixel[px];
        } else {
            // Both opaque: sprite priority attribute decides
            // 0 = sprite in front of BG, 1 = sprite behind BG
            output = spr_priority[px] ? bg_pixel[px] : spr_pixel[px];
        }

        ppu->framebuf[sl * NES_WIDTH + px] = output;
    }
}

void ppu_step(PPU* ppu) {
    bool rendering_enabled = (ppu->mask & 0x18) != 0;

    // Visible scanlines (0-239)
    if (ppu->scanline < 240 && rendering_enabled) {
        if (ppu->dot == 0) {
            ppu_render_scanline(ppu);
        }
        if (ppu->dot == 256) {
            ppu_increment_scroll_y(ppu);
        }
        if (ppu->dot == 257) {
            ppu_copy_horizontal_bits(ppu);
        }
    }

    // Pre-render scanline (261)
    if (ppu->scanline == PRE_RENDER_SCANLINE) {
        if (ppu->dot == 1) {
            ppu->status &= ~0xE0; // Clear vblank, sprite 0 hit, overflow
            ppu->nmi_occurred = false;
        }
        if (rendering_enabled && ppu->dot >= 280 && ppu->dot <= 304) {
            ppu_copy_vertical_bits(ppu);
        }
    }

    // Vblank start (scanline 241, dot 1)
    if (ppu->scanline == VBLANK_SCANLINE && ppu->dot == 1) {
        ppu->status |= 0x80;
        ppu->nmi_occurred = true;
        ppu->frame_complete = true;
    }

    // Advance dot/scanline
    ppu->dot++;
    if (ppu->dot > 340) {
        ppu->dot = 0;
        ppu->scanline++;
        if (ppu->scanline > 261) {
            ppu->scanline = 0;
        }
    }
}

bool ppu_nmi_pending(PPU* ppu) { return ppu->nmi_occurred && ppu->nmi_output; }

void ppu_clear_nmi(PPU* ppu) { ppu->nmi_occurred = false; }
