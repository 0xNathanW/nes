#include "cpu.h"
#include "bus.h"
#include <stdint.h>
#include <stdio.h>

#define STACK_BASE 0x0100

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

// Stack operations. Stack lives at $0100-$01FF.
void cpu_push_byte(CPU_6502* cpu, uint8_t data) {
    bus_write_byte(cpu->bus, STACK_BASE | cpu->regs.sp, data);
    cpu->regs.sp--;
}

uint8_t cpu_pop_byte(CPU_6502* cpu) {
    cpu->regs.sp++;
    return bus_read_byte(cpu->bus, STACK_BASE | cpu->regs.sp);
}

void cpu_push_word(CPU_6502* cpu, uint16_t data) {
    cpu_push_byte(cpu, data >> 8);
    cpu_push_byte(cpu, data & 0xFF);
}

uint16_t cpu_pop_word(CPU_6502* cpu) {
    uint8_t lo = cpu_pop_byte(cpu);
    uint8_t hi = cpu_pop_byte(cpu);
    return (hi << 8) | lo;
}

// Get the address of the operand and advance PC.
uint16_t cpu_get_op_addr(CPU_6502* cpu, AddressingMode mode) {
    uint8_t lo, hi, zp_ptr;
    uint16_t addr, ptr;

    switch (mode) {
    case IMPLIED:
    case ACCUMULATOR:
        return 0;

    // 1-byte
    case IMMEDIATE:
        addr = cpu->regs.pc++;
        return addr;
    case ZERO_PAGE:
        addr = bus_read_byte(cpu->bus, cpu->regs.pc++);
        return addr;
    case ZERO_PAGE_X:
        addr = (bus_read_byte(cpu->bus, cpu->regs.pc++) + cpu->regs.x) & 0xFF;
        return addr;
    case ZERO_PAGE_Y:
        addr = (bus_read_byte(cpu->bus, cpu->regs.pc++) + cpu->regs.y) & 0xFF;
        return addr;
    case RELATIVE:
        addr = cpu->regs.pc++;
        return addr;
    case INDEXED_INDIRECT:
        zp_ptr = (bus_read_byte(cpu->bus, cpu->regs.pc++) + cpu->regs.x) & 0xFF;
        return bus_read_byte(cpu->bus, zp_ptr) |
               (bus_read_byte(cpu->bus, (zp_ptr + 1) & 0xFF) << 8);
    case INDIRECT_INDEXED:
        zp_ptr = bus_read_byte(cpu->bus, cpu->regs.pc++);
        return (bus_read_byte(cpu->bus, zp_ptr) |
                (bus_read_byte(cpu->bus, (zp_ptr + 1) & 0xFF) << 8)) +
               cpu->regs.y;

    // 2-byte
    case ABSOLUTE:
        addr = bus_read_word(cpu->bus, cpu->regs.pc);
        cpu->regs.pc += 2;
        return addr;
    case ABSOLUTE_X:
        addr = bus_read_word(cpu->bus, cpu->regs.pc) + cpu->regs.x;
        cpu->regs.pc += 2;
        return addr;
    case ABSOLUTE_Y:
        addr = bus_read_word(cpu->bus, cpu->regs.pc) + cpu->regs.y;
        cpu->regs.pc += 2;
        return addr;
    case INDIRECT:
        lo = bus_read_byte(cpu->bus, cpu->regs.pc);
        hi = bus_read_byte(cpu->bus, cpu->regs.pc + 1);
        cpu->regs.pc += 2;
        ptr = lo | (hi << 8);
        if (lo == 0xFF) {
            // Page boundary: http://www.6502.org/tutorials/6502opcodes.html#JMP
            return bus_read_byte(cpu->bus, ptr) |
                   (bus_read_byte(cpu->bus, ptr & 0xFF00) << 8);
        }
        return bus_read_word(cpu->bus, ptr);

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

static void update_zn_flags(CPU_6502* cpu, uint8_t value) {
    cpu_set_flag(cpu, FLAG_ZERO, value == 0);
    cpu_set_flag(cpu, FLAG_NEGATIVE, value & 0x80);
}

void cpu_step(CPU_6502* cpu) {
    uint8_t opcode = bus_read_byte(cpu->bus, cpu->regs.pc++);

    switch (opcode) {

    // NOP
    case 0xEA:
        break;

    // JMP - Jump to address
    case 0x4C:
        cpu->regs.pc = cpu_get_op_addr(cpu, ABSOLUTE);
        break;
    case 0x6C:
        cpu->regs.pc = cpu_get_op_addr(cpu, INDIRECT);
        break;

    // LDA - Loads value into accumulator
    case 0xA9:
        cpu->regs.acc =
            bus_read_byte(cpu->bus, cpu_get_op_addr(cpu, IMMEDIATE));
        update_zn_flags(cpu, cpu->regs.acc);
        break;
    case 0xA5:
        cpu->regs.acc =
            bus_read_byte(cpu->bus, cpu_get_op_addr(cpu, ZERO_PAGE));
        update_zn_flags(cpu, cpu->regs.acc);
        break;
    case 0xB5:
        cpu->regs.acc =
            bus_read_byte(cpu->bus, cpu_get_op_addr(cpu, ZERO_PAGE_X));
        update_zn_flags(cpu, cpu->regs.acc);
        break;
    case 0xAD:
        cpu->regs.acc = bus_read_byte(cpu->bus, cpu_get_op_addr(cpu, ABSOLUTE));
        update_zn_flags(cpu, cpu->regs.acc);
        break;
    case 0xBD:
        cpu->regs.acc =
            bus_read_byte(cpu->bus, cpu_get_op_addr(cpu, ABSOLUTE_X));
        update_zn_flags(cpu, cpu->regs.acc);
        break;
    case 0xB9:
        cpu->regs.acc =
            bus_read_byte(cpu->bus, cpu_get_op_addr(cpu, ABSOLUTE_Y));
        update_zn_flags(cpu, cpu->regs.acc);
        break;
    case 0xA1:
        cpu->regs.acc =
            bus_read_byte(cpu->bus, cpu_get_op_addr(cpu, INDEXED_INDIRECT));
        update_zn_flags(cpu, cpu->regs.acc);
        break;
    case 0xB1:
        cpu->regs.acc =
            bus_read_byte(cpu->bus, cpu_get_op_addr(cpu, INDIRECT_INDEXED));
        update_zn_flags(cpu, cpu->regs.acc);
        break;

    // LDX - Load value into X register
    case 0xA2:
        cpu->regs.x = bus_read_byte(cpu->bus, cpu_get_op_addr(cpu, IMMEDIATE));
        update_zn_flags(cpu, cpu->regs.x);
        break;
    case 0xA6:
        cpu->regs.x = bus_read_byte(cpu->bus, cpu_get_op_addr(cpu, ZERO_PAGE));
        update_zn_flags(cpu, cpu->regs.x);
        break;
    case 0xB6:
        cpu->regs.x =
            bus_read_byte(cpu->bus, cpu_get_op_addr(cpu, ZERO_PAGE_Y));
        update_zn_flags(cpu, cpu->regs.x);
        break;
    case 0xAE:
        cpu->regs.x = bus_read_byte(cpu->bus, cpu_get_op_addr(cpu, ABSOLUTE));
        update_zn_flags(cpu, cpu->regs.x);
        break;
    case 0xBE:
        cpu->regs.x = bus_read_byte(cpu->bus, cpu_get_op_addr(cpu, ABSOLUTE_Y));
        update_zn_flags(cpu, cpu->regs.x);
        break;

    // LDY - Load value into Y register
    case 0xA0:
        cpu->regs.y = bus_read_byte(cpu->bus, cpu_get_op_addr(cpu, IMMEDIATE));
        update_zn_flags(cpu, cpu->regs.y);
        break;
    case 0xA4:
        cpu->regs.y = bus_read_byte(cpu->bus, cpu_get_op_addr(cpu, ZERO_PAGE));
        update_zn_flags(cpu, cpu->regs.y);
        break;
    case 0xB4:
        cpu->regs.y =
            bus_read_byte(cpu->bus, cpu_get_op_addr(cpu, ZERO_PAGE_X));
        update_zn_flags(cpu, cpu->regs.y);
        break;
    case 0xAC:
        cpu->regs.y = bus_read_byte(cpu->bus, cpu_get_op_addr(cpu, ABSOLUTE));
        update_zn_flags(cpu, cpu->regs.y);
        break;
    case 0xBC:
        cpu->regs.y = bus_read_byte(cpu->bus, cpu_get_op_addr(cpu, ABSOLUTE_X));
        update_zn_flags(cpu, cpu->regs.y);
        break;

    default:
        printf("unimplemented opcode: %02X at %04X\n", opcode,
               cpu->regs.pc - 1);
        break;
    }
}
