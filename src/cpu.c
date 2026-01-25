#include "cpu.h"
#include "bus.h"
#include <stdint.h>
#include <stdio.h>

// Cold boot.
void cpu_power_on(CPU_6502* cpu) {
    cpu->regs.acc = 0;
    cpu->regs.x = 0;
    cpu->regs.y = 0;
    cpu->regs.sp = 0xFD;
    cpu->regs.p = 0x34;
    cpu->regs.pc = bus_read_word(cpu->bus, 0xFFFC);
}

// Warm reset.
void cpu_reset(CPU_6502* cpu) {
    cpu->regs.sp -= 3;
    cpu->regs.p |= FLAG_INTERRUPT;
    cpu->regs.pc = bus_read_word(cpu->bus, 0xFFFC);
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

// Get the address of the operand for the given addressing mode.
uint16_t cpu_get_op_addr(CPU_6502* cpu, AddressingMode mode) {
    switch (mode) {
    case IMMEDIATE:
        return cpu->regs.pc;
    case ZERO_PAGE:
        return bus_read_byte(cpu->bus, cpu->regs.pc);
    case ZERO_PAGE_X:
        return (bus_read_byte(cpu->bus, cpu->regs.pc) + cpu->regs.x) & 0xFF;
    case ZERO_PAGE_Y:
        return (bus_read_byte(cpu->bus, cpu->regs.pc) + cpu->regs.y) & 0xFF;
    case ABSOLUTE:
        return bus_read_word(cpu->bus, cpu->regs.pc);
    case ABSOLUTE_X:
        return bus_read_word(cpu->bus, cpu->regs.pc) + cpu->regs.x;
    case ABSOLUTE_Y:
        return bus_read_word(cpu->bus, cpu->regs.pc) + cpu->regs.y;
    default:
        printf("unimplemented addressing mode: %d\n", mode);
        return 0;
    }
}

void cpu_trace(CPU_6502* cpu) {
    uint8_t opcode = bus_read_byte(cpu->bus, cpu->regs.pc);
    fprintf(stderr, "%04X  %02X  A:%02X X:%02X Y:%02X P:%02X SP:%02X\n",
            cpu->regs.pc, opcode, cpu->regs.acc, cpu->regs.x, cpu->regs.y,
            cpu->regs.p, cpu->regs.sp);
}

void cpu_step(CPU_6502* cpu) {
    uint8_t opcode = bus_read_byte(cpu->bus, cpu->regs.pc++);
    printf("opcode: %02X\n", opcode);
}
