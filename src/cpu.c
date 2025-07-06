#include "cpu.h"
#include <string.h>
#include "bus.h"
#include <stdio.h>

void cpu_init(CPU_6502* cpu) {
    // DO i have to do this?
    memset(cpu, 0, sizeof(CPU_6502));
    memset(cpu->ram, 0, sizeof(cpu->ram));
    cpu->regs.pc = bus_read_byte(cpu->bus, 0xFFFC);
    cpu->regs.sp = 0xFD;
    cpu->regs.p = 0x34;
}

void cpu_set_flag(CPU_6502* cpu, uint8_t flag, bool value) {
    if (value) {
        cpu->regs.p |= flag;
    } else {
        cpu->regs.p &= ~flag;
    }
}

bool cpu_get_flag(CPU_6502* cpu, uint8_t flag) {
    return (cpu->regs.p & flag) != 0;
}

void cpu_step(CPU_6502* cpu) {
    uint8_t opcode = bus_read_byte(cpu->bus, cpu->regs.pc++);
    printf("opcode: %02X\n", opcode);
}