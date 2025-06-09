#ifndef CPU_H
#define CPU_H

#include <stdint.h>
#include <stdbool.h>
#include "ines.h"

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

/*
Address 	Size  	  Description
$0000 	  0x800 	  2KB of work RAM
$0800 	  0x800 	  Mirror of $000-$7FF
$1000 	  0x800 	  Mirror of $000-$7FF
$1800 	  0x800 	  Mirror of $000-$7FF
$2000 	  0x8 	    PPU Ctrl Registers
$2008 	  0x1FF8 	  *Mirror of $2000-$2007
$4000 	  0x20 	    Registers (Mostly APU)
$4020 	  0x1FDF 	  Cartridge Expansion ROM
$6000 	  0x2000 	  SRAM
$8000 	  0x4000 	  PRG-ROM
$C000 	  0x4000 	  PRG-ROM 
*/
#define RAM_SIZE 0x0800
#define RAM_START 0x0000
#define RAM_END 0x07FF
#define RAM_MIRROR_END 0x1FF
#define PPU_REGS_START 0x2000
#define PPU_REGS_END 0x2007

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
  
    uint8_t ram[RAM_SIZE];
    INES_Cart *cart;
} CPU;

CPU* init_cpu(INES_Cart* cart);
void free_cpu(CPU* cpu);
void reset_cpu(CPU* cpu);
void step_cpu(CPU* cpu);

uint8_t cpu_read_byte(CPU* cpu, uint16_t addr);
uint16_t cpu_read_word(CPU* cpu, uint16_t addr);

void cpu_write_byte(CPU* cpu, uint16_t addr, uint8_t byte);
void cpu_write_word(CPU* cpu, uint16_t adr, uint16_t word);

// Stack functions.
// TODO: maybe need push/pop word (2 bytes)
void stack_push(CPU* cpu, uint8_t data); 
uint8_t stack_pop(CPU* cpu);

#endif
