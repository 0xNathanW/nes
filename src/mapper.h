#ifndef MAPPER_H
#define MAPPER_H

#include <stdint.h>

#define MIRROR_HORIZONTAL   0
#define MIRROR_VERTICAL     1
#define MIRROR_ONESCREEN_LO 2
#define MIRROR_ONESCREEN_HI 3

struct Cartridge;

typedef struct Mapper {
    uint8_t (*read_prg)(struct Cartridge*, uint16_t addr);
    void    (*write_prg)(struct Cartridge*, uint16_t addr, uint8_t data);
    uint8_t (*read_chr)(struct Cartridge*, uint16_t addr);
    void    (*write_chr)(struct Cartridge*, uint16_t addr, uint8_t data);
    void    (*scanline_counter)(struct Cartridge*);
} Mapper;

void mapper0_init(struct Cartridge*);
void mapper1_init(struct Cartridge*);
void mapper2_init(struct Cartridge*);
void mapper4_init(struct Cartridge*);

#endif
