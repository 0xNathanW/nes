#include "cpu.h"
#include <string.h>

void reset_cpu(CPU* cpu) {
    cpu->pc = 0x8000;
    cpu->acc = 0;
    cpu->x_index = 0;
    cpu->y_index = 0;
    cpu->flag = 0x34;
    cpu->sp = 0xFD;
    // Clear RAM.
    memset(cpu->ram, 0, 0x0800);
}



