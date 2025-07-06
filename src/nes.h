#ifndef NES_H
#define NES_H

#include "cpu.h"
#include "ines.h"

typedef struct {
    CPU cpu;
    INES_Cart* cart;
} NES;

NES* create_nes(INES_Cart* cart);
void destroy_nes(NES* nes);

// uint8_t bus_read_byte(uint16_t addr);
// void bus_write_byte(uint16_t addr, uint8_t data);

#endif
