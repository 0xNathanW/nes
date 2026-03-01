#include "cart.h"
#include "cpu.h"
#include "log.h"
#include "nes.h"

#define NESTEST_INSTRUCTION_LIMIT 8991
#define LOG_PATH "test_roms/nestest_simple.log"
#define MAX_LINE 128

typedef struct {
    uint16_t pc;
    uint8_t opcode;
    uint8_t a, x, y, p, sp;
} ExpectedState;

static int parse_log_line(const char* line, ExpectedState* state) {
    unsigned pc, opcode, a, x, y, p, sp;
    int n = sscanf(line, "%X %X A:%X X:%X Y:%X P:%X SP:%X",
                   &pc, &opcode, &a, &x, &y, &p, &sp);
    if (n != 7)
        return 0;
    state->pc = (uint16_t)pc;
    state->opcode = (uint8_t)opcode;
    state->a = (uint8_t)a;
    state->x = (uint8_t)x;
    state->y = (uint8_t)y;
    state->p = (uint8_t)p;
    state->sp = (uint8_t)sp;
    return 1;
}

static int verify_state(CPU_6502* cpu, ExpectedState* expected, int line_num,
                        ExpectedState* prev) {
    uint8_t opcode = bus_read_byte(cpu->bus, cpu->regs.pc);
    int pass = 1;

    if (cpu->regs.pc != expected->pc) pass = 0;
    if (opcode != expected->opcode) pass = 0;
    if (cpu->regs.acc != expected->a) pass = 0;
    if (cpu->regs.x != expected->x) pass = 0;
    if (cpu->regs.y != expected->y) pass = 0;
    if (cpu->regs.p != expected->p) pass = 0;
    if (cpu->regs.sp != expected->sp) pass = 0;

    if (!pass) {
        if (prev) {
            printf("instruction %d: %04X  %02X  A:%02X X:%02X Y:%02X P:%02X SP:%02X\n",
                   line_num - 1, prev->pc, prev->opcode, prev->a, prev->x,
                   prev->y, prev->p, prev->sp);
        }
        printf("MISMATCH at instruction %d:\n", line_num);
        printf("  expected: %04X  %02X  A:%02X X:%02X Y:%02X P:%02X SP:%02X\n",
               expected->pc, expected->opcode, expected->a, expected->x,
               expected->y, expected->p, expected->sp);
        printf("  actual:   %04X  %02X  A:%02X X:%02X Y:%02X P:%02X SP:%02X\n",
               cpu->regs.pc, opcode, cpu->regs.acc, cpu->regs.x,
               cpu->regs.y, cpu->regs.p, cpu->regs.sp);

        // Show which fields differ
        printf("  diff:    ");
        if (cpu->regs.pc != expected->pc) printf(" PC");
        if (opcode != expected->opcode) printf(" OP");
        if (cpu->regs.acc != expected->a) printf(" A");
        if (cpu->regs.x != expected->x) printf(" X");
        if (cpu->regs.y != expected->y) printf(" Y");
        if (cpu->regs.p != expected->p) printf(" P");
        if (cpu->regs.sp != expected->sp) printf(" SP");
        printf("\n");
    }

    return pass;
}

int main(int argc, char* argv[]) {
    const char* rom_path = (argc > 1) ? argv[1] : "../test_roms/nestest.nes";

    NES* nes = nes_create();
    if (!nes) {
        LOG_ERROR("could not create NES");
        return 1;
    }

    if (!nes_load_cartridge(nes, rom_path)) {
        LOG_ERROR("could not load cartridge: %s", rom_path);
        nes_destroy(nes);
        return 1;
    }

    print_cart_info(nes->cartridge);

    FILE* log = fopen(LOG_PATH, "r");
    if (!log) {
        LOG_ERROR("could not open log: %s", LOG_PATH);
        nes_destroy(nes);
        return 1;
    }

    // For nestest automation mode, start at $C000
    nes->cpu.regs.pc = 0xC000;

    char line[MAX_LINE];
    int line_num = 0;
    ExpectedState prev = {0};

    for (int i = 0; i < NESTEST_INSTRUCTION_LIMIT; i++) {
        if (!fgets(line, sizeof(line), log)) {
            LOG_WARN("log ended early at instruction %d", i + 1);
            break;
        }
        line_num++;

        ExpectedState expected;
        if (!parse_log_line(line, &expected)) {
            LOG_ERROR("could not parse log line %d: %s", line_num, line);
            break;
        }

        if (!verify_state(&nes->cpu, &expected, line_num,
                          line_num > 1 ? &prev : NULL)) {
            break;
        }

        prev = expected;
        cpu_step(&nes->cpu);
    }

    printf("verified %d instructions\n", line_num);

    fclose(log);
    nes_destroy(nes);
    return 0;
}
