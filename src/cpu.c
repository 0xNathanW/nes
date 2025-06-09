#include "cpu.h"
#include "ines.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

CPU* init_cpu(INES_Cart* cart) {

    CPU* cpu = malloc(sizeof(CPU));
    if (!cpu) {
        printf("error: unable to allocate memory for cpu\n");
        return NULL;
    }

    memset(cpu, 0, sizeof(CPU));
    cpu->cart = cart;
    reset_cpu(cpu);

    return cpu;
}

void free_cpu(CPU* cpu) {
    if (cpu->cart) {
        free(cpu->cart);
    }
    if (cpu) {
        free(cpu);
    }
}
