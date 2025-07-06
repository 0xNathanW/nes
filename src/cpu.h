#ifndef CPU_H
#define CPU_H

#include <stdint.h>
#include <stdbool.h>

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

#define FLAG_CARRY     0x01
#define FLAG_ZERO      0x02
#define FLAG_INTERRUPT 0x03
#define FLAG_DECIMAL   0x04
#define FLAG_BREAK     0x05
#define FLAG_OVERFLOW  0x01
#define FLAG_NEGATIVE  0x01

typedef struct {
    // Registers:
    // The accumulator can read and write to memory.
    // It is used to store arithmetic and logic results such as addition and
    // subtraction.
    uint8_t acc;
    // The x index is can read and write to memory. It is used primarily as a
    // counter in loops, or for addressing memory, but can also temporarily
    // store data like the accumulator.
    uint8_t x_index;
    // Much like the x index, however they are not completely interchangeable.
    // Some operations are only available for each register.
    uint8_t y_index;
    // The register holds value of 7 different flags which can only have a value
    // of 0 or 1 and hence can be represented in a single register. The bits
    // represent the status of the processor.
    uint8_t flag;
    // The processor supports a 256 byte stack located between $0100 and $01FF.
    // The stack pointer is an 8 bit register and holds the low 8 bits of the
    // next free location on the stack. The location of the stack is fixed and
    // cannot be moved.
    uint8_t sp;
    // This is a 16-bit register unlike other registers which are only 8-bit in
    // length, it indicates where the processor is in the program sequence.
    uint16_t pc;
  
    uint8_t ram[0x0800];
} CPU;

void reset_cpu(CPU* cpu);

#endif
