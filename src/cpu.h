#ifndef CPU_H
#define CPU_H

#include <stdint.h>
#include <stdbool.h>
#include "bus.h"

/* Flags:
    Carry Flag:
    The carry flag is set if the last operation caused an overflow from bit 7 of
    the result or an underflow from bit 0. This condition is set during
    arithmetic, comparison and during logical shifts. It can be explicitly set
    using the 'Set Carry Flag' (SEC) instruction and cleared with 'Clear Carry
    Flag' (CLC).

    Zero Flag
    The zero flag is set if the result of the last operation as was zero.

    Interrupt Disable
    The interrupt disable flag is set if the program has executed a 'Set
    Interrupt Disable' (SEI) instruction. While this flag is set the processor
    will not respond to interrupts from devices until it is cleared by a 'Clear
    Interrupt Disable' (CLI) instruction.

    Decimal Mode
    While the decimal mode flag is set the processor will obey the rules of
    Binary Coded Decimal (BCD) arithmetic during addition and subtraction. The
    flag can be explicitly set using 'Set Decimal Flag' (SED) and cleared with
    'Clear Decimal Flag' (CLD).

    Break Command
    The break command bit is set when a BRK instruction has been executed and an
    interrupt has been generated to process it.

    Overflow Flag
    The overflow flag is set during arithmetic operations if the result has
    yielded an invalid 2's complement result (e.g. adding to positive numbers and
    ending up with a negative result: 64 + 64 => -128). It is determined by
    looking at the carry between bits 6 and 7 and between bit 7 and the carry
    flag.

    Negative Flag
    The negative flag is set if the result of the last operation had bit 7 set
    to a one.
*/

/*
7  bit  0
---- ----
NV1B DIZC
|||| ||||
|||| |||+- Carry
|||| ||+-- Zero
|||| |+--- Interrupt Disable
|||| +---- Decimal
|||+------ (No CPU effect; see: the B flag)
||+------- (No CPU effect; always pushed as 1)
|+-------- Overflow
+--------- Negative
*/

#define FLAG_CARRY     (1 << 0)
#define FLAG_ZERO      (1 << 1)
#define FLAG_INTERRUPT (1 << 2)
#define FLAG_DECIMAL   (1 << 3)
#define FLAG_BREAK     (1 << 4)
#define FLAG_OVERFLOW  (1 << 6)
#define FLAG_NEGATIVE  (1 << 7)

typedef struct Registers {
    // Registers:
    // The accumulator can read and write to memory.
    // It is used to store arithmetic and logic results such as addition and
    // subtraction.
    uint8_t acc;
    // The x index is can read and write to memory. It is used primarily as a
    // counter in loops, or for addressing memory, but can also temporarily
    // store data like the accumulator.
    uint8_t x;
    // Much like the x index, however they are not completely interchangeable.
    // Some operations are only available for each register.
    uint8_t y;
    // The register holds value of 7 different flags which can only have a value
    // of 0 or 1 and hence can be represented in a single register. The bits
    // represent the status of the processor.
    uint8_t p;
    // The processor supports a 256 byte stack located between $0100 and $01FF.
    // The stack pointer is an 8 bit register and holds the low 8 bits of the
    // next free location on the stack. The location of the stack is fixed and
    // cannot be moved.
    uint8_t sp;
    // This is a 16-bit register unlike other registers which are only 8-bit in
    // length, it indicates where the processor is in the program sequence.
    uint16_t pc;  
} Registers;

// Addressing mode is a property of an instruction that defines how the CPU
// should interpret the next 1 or 2 bytes of the instruction stream.
typedef enum AddressingMode {
    // No operand.
    IMPLIED,
    // Operand is the actual value.
    IMMEDIATE,
    // Address in first 256 bytes of memory (0x0000 - 0x00FF).
    ZERO_PAGE,
    // Zero page with X offset.
    ZERO_PAGE_X,
    // Zero page with Y offset.
    ZERO_PAGE_Y,
    // Full 16-bit address.
    ABSOLUTE,
    // Absolute with X offset.
    ABSOLUTE_X,
    // Absolute with Y offset.
    ABSOLUTE_Y,
} AddressingMode;

typedef struct CPU_6502 {
    Registers regs;
    Bus* bus;
} CPU_6502;

void cpu_init(CPU_6502* cpu);
void cpu_power_on(CPU_6502* cpu);
void cpu_reset(CPU_6502* cpu);
void cpu_step(CPU_6502* cpu);

void cpu_set_flag(CPU_6502* cpu, uint8_t flag, bool value);
bool cpu_get_flag(CPU_6502* cpu, uint8_t flag);

#endif
