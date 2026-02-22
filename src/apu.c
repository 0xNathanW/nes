#include "apu.h"
#include <string.h>

// clang-format off
static const uint8_t DUTY_TABLE[4][8] = {
    {0, 1, 0, 0, 0, 0, 0, 0},
    {0, 1, 1, 0, 0, 0, 0, 0},
    {0, 1, 1, 1, 1, 0, 0, 0},
    {1, 0, 0, 1, 1, 1, 1, 1},
};

static const uint8_t LENGTH_TABLE[32] = {
    10,254, 20,  2, 40,  4, 80,  6,160,  8, 60, 10, 14, 12, 26, 14,
    12, 16, 24, 18, 48, 20, 96, 22,192, 24, 72, 26, 16, 28, 32, 30,
};

static const uint16_t NOISE_PERIOD_TABLE[16] = {
    4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068,
};

static const uint8_t TRIANGLE_SEQ[32] = {
    15,14,13,12,11,10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0,
     0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,
};
// clang-format on

static float pulse_table[31];
static float tnd_table[203];
static bool tables_initialized = false;

static void init_mixing_tables(void) {
    if (tables_initialized)
        return;
    pulse_table[0] = 0.0f;
    for (int i = 1; i < 31; i++) {
        pulse_table[i] = 95.52f / (8128.0f / i + 100.0f);
    }
    tnd_table[0] = 0.0f;
    for (int i = 1; i < 203; i++) {
        tnd_table[i] = 163.67f / (24329.0f / i + 100.0f);
    }
    tables_initialized = true;
}

void apu_init(APU* apu) {
    memset(apu, 0, sizeof(APU));
    apu->noise.shift_register = 1;
    apu->cycles_per_sample = 1789773.0 / APU_SAMPLE_RATE;
    init_mixing_tables();
}

static void clock_envelope_pulse(PulseChannel* ch) {
    if (ch->envelope_start) {
        ch->envelope_start = false;
        ch->envelope_decay = 15;
        ch->envelope_divider = ch->envelope_volume;
    } else {
        if (ch->envelope_divider == 0) {
            ch->envelope_divider = ch->envelope_volume;
            if (ch->envelope_decay > 0) {
                ch->envelope_decay--;
            } else if (ch->envelope_loop) {
                ch->envelope_decay = 15;
            }
        } else {
            ch->envelope_divider--;
        }
    }
}

static void clock_envelope_noise(NoiseChannel* ch) {
    if (ch->envelope_start) {
        ch->envelope_start = false;
        ch->envelope_decay = 15;
        ch->envelope_divider = ch->envelope_volume;
    } else {
        if (ch->envelope_divider == 0) {
            ch->envelope_divider = ch->envelope_volume;
            if (ch->envelope_decay > 0) {
                ch->envelope_decay--;
            } else if (ch->envelope_loop) {
                ch->envelope_decay = 15;
            }
        } else {
            ch->envelope_divider--;
        }
    }
}

// --- Sweep ---

static uint16_t sweep_target_period(PulseChannel* ch, int channel) {
    uint16_t change = ch->timer_period >> ch->sweep_shift;
    if (ch->sweep_negate) {
        // Pulse 1: one's complement, Pulse 2: two's complement
        if (channel == 0) {
            return ch->timer_period - change - 1;
        }
        return ch->timer_period - change;
    }
    return ch->timer_period + change;
}

static bool sweep_muting(PulseChannel* ch, int channel) {
    return ch->timer_period < 8 || sweep_target_period(ch, channel) > 0x7FF;
}

static void clock_sweep(PulseChannel* ch, int channel) {
    uint16_t target = sweep_target_period(ch, channel);
    if (ch->sweep_divider == 0 && ch->sweep_enabled &&
        !sweep_muting(ch, channel)) {
        ch->timer_period = target;
    }
    if (ch->sweep_divider == 0 || ch->sweep_reload) {
        ch->sweep_divider = ch->sweep_period;
        ch->sweep_reload = false;
    } else {
        ch->sweep_divider--;
    }
}

static void clock_linear_counter(TriangleChannel* ch) {
    if (ch->linear_counter_reload_flag) {
        ch->linear_counter = ch->linear_counter_reload_value;
    } else if (ch->linear_counter > 0) {
        ch->linear_counter--;
    }
    if (!ch->control_flag) {
        ch->linear_counter_reload_flag = false;
    }
}

static void clock_length_pulse(PulseChannel* ch) {
    if (!ch->envelope_loop && ch->length_counter > 0) {
        ch->length_counter--;
    }
}

static void clock_length_triangle(TriangleChannel* ch) {
    if (!ch->control_flag && ch->length_counter > 0) {
        ch->length_counter--;
    }
}

static void clock_length_noise(NoiseChannel* ch) {
    if (!ch->envelope_loop && ch->length_counter > 0) {
        ch->length_counter--;
    }
}

static void clock_quarter_frame(APU* apu) {
    clock_envelope_pulse(&apu->pulse[0]);
    clock_envelope_pulse(&apu->pulse[1]);
    clock_envelope_noise(&apu->noise);
    clock_linear_counter(&apu->triangle);
}

static void clock_half_frame(APU* apu) {
    clock_quarter_frame(apu);
    clock_length_pulse(&apu->pulse[0]);
    clock_length_pulse(&apu->pulse[1]);
    clock_length_triangle(&apu->triangle);
    clock_length_noise(&apu->noise);
    clock_sweep(&apu->pulse[0], 0);
    clock_sweep(&apu->pulse[1], 1);
}

static void step_frame_counter(APU* apu) {
    apu->frame_counter++;

    if (apu->frame_mode == 0) {
        // 4-step mode
        switch (apu->frame_counter) {
        case 3729:
            clock_quarter_frame(apu);
            break;
        case 7457:
            clock_half_frame(apu);
            break;
        case 11186:
            clock_quarter_frame(apu);
            break;
        case 14915:
            clock_half_frame(apu);
            if (!apu->frame_irq_inhibit) {
                apu->frame_irq = true;
                apu->irq_pending = true;
            }
            apu->frame_counter = 0;
            break;
        }
    } else {
        // 5-step mode (no IRQ)
        switch (apu->frame_counter) {
        case 3729:
            clock_quarter_frame(apu);
            break;
        case 7457:
            clock_half_frame(apu);
            break;
        case 11186:
            clock_quarter_frame(apu);
            break;
        case 18641:
            clock_half_frame(apu);
            apu->frame_counter = 0;
            break;
        }
    }
}

static void step_pulse_timer(PulseChannel* ch) {
    if (ch->timer_value == 0) {
        ch->timer_value = ch->timer_period;
        ch->duty_pos = (ch->duty_pos + 1) & 7;
    } else {
        ch->timer_value--;
    }
}

static void step_triangle_timer(TriangleChannel* ch) {
    if (ch->timer_value == 0) {
        ch->timer_value = ch->timer_period;
        if (ch->length_counter > 0 && ch->linear_counter > 0) {
            ch->seq_pos = (ch->seq_pos + 1) & 31;
        }
    } else {
        ch->timer_value--;
    }
}

static void step_noise_timer(NoiseChannel* ch) {
    if (ch->timer_value == 0) {
        ch->timer_value = ch->timer_period;
        uint8_t bit = ch->mode ? 6 : 1;
        uint16_t feedback =
            (ch->shift_register & 1) ^ ((ch->shift_register >> bit) & 1);
        ch->shift_register >>= 1;
        ch->shift_register |= (feedback << 14);
    } else {
        ch->timer_value--;
    }
}

static uint8_t pulse_output(PulseChannel* ch, int channel) {
    if (!ch->enabled)
        return 0;
    if (ch->length_counter == 0)
        return 0;
    if (DUTY_TABLE[ch->duty][ch->duty_pos] == 0)
        return 0;
    if (sweep_muting(ch, channel))
        return 0;
    return ch->constant_volume ? ch->envelope_volume : ch->envelope_decay;
}

static uint8_t triangle_output(TriangleChannel* ch) {
    if (!ch->enabled)
        return 0;
    if (ch->length_counter == 0)
        return 0;
    if (ch->linear_counter == 0)
        return 0;
    return TRIANGLE_SEQ[ch->seq_pos];
}

static uint8_t noise_output(NoiseChannel* ch) {
    if (!ch->enabled)
        return 0;
    if (ch->length_counter == 0)
        return 0;
    if (ch->shift_register & 1)
        return 0;
    return ch->constant_volume ? ch->envelope_volume : ch->envelope_decay;
}

void apu_step(APU* apu) {
    step_frame_counter(apu);

    // Triangle timer runs every CPU cycle
    step_triangle_timer(&apu->triangle);

    // Pulse and noise timers run every other CPU cycle
    apu->odd_cycle = !apu->odd_cycle;
    if (apu->odd_cycle) {
        step_pulse_timer(&apu->pulse[0]);
        step_pulse_timer(&apu->pulse[1]);
        step_noise_timer(&apu->noise);
    }

    // Mix channels
    uint8_t p1 = pulse_output(&apu->pulse[0], 0);
    uint8_t p2 = pulse_output(&apu->pulse[1], 1);
    uint8_t tri = triangle_output(&apu->triangle);
    uint8_t noi = noise_output(&apu->noise);
    uint8_t dmc_out = apu->dmc.output_level;

    float pulse_out = pulse_table[p1 + p2];
    int tnd_index = 3 * tri + 2 * noi + dmc_out;
    if (tnd_index > 202)
        tnd_index = 202;
    float tnd_out = tnd_table[tnd_index];

    float sample = pulse_out + tnd_out;

    // Downsample: accumulate and emit at 44100 Hz
    apu->sample_accumulator += sample;
    apu->sample_count++;
    apu->sample_period += 1.0;

    if (apu->sample_period >= apu->cycles_per_sample) {
        apu->sample_period -= apu->cycles_per_sample;
        float avg = (float)(apu->sample_accumulator / apu->sample_count);

        // Write to ring buffer
        uint32_t next = (apu->buffer_write + 1) % APU_BUFFER_SIZE;
        if (next != apu->buffer_read) {
            apu->buffer[apu->buffer_write] = avg;
            apu->buffer_write = next;
        }

        apu->sample_accumulator = 0.0;
        apu->sample_count = 0;
    }
}

static void write_pulse(PulseChannel* ch, uint16_t reg, uint8_t data) {
    switch (reg) {
    case 0: // $4000/$4004
        ch->duty = (data >> 6) & 3;
        ch->envelope_loop = (data >> 5) & 1;
        ch->constant_volume = (data >> 4) & 1;
        ch->envelope_volume = data & 0x0F;
        break;
    case 1: // $4001/$4005
        ch->sweep_enabled = (data >> 7) & 1;
        ch->sweep_period = (data >> 4) & 7;
        ch->sweep_negate = (data >> 3) & 1;
        ch->sweep_shift = data & 7;
        ch->sweep_reload = true;
        break;
    case 2: // $4002/$4006
        ch->timer_period = (ch->timer_period & 0x700) | data;
        break;
    case 3: // $4003/$4007
        ch->timer_period = (ch->timer_period & 0xFF) | ((uint16_t)(data & 7) << 8);
        if (ch->enabled) {
            ch->length_counter = LENGTH_TABLE[data >> 3];
        }
        ch->duty_pos = 0;
        ch->envelope_start = true;
        break;
    }
}

void apu_write_register(APU* apu, uint16_t addr, uint8_t data) {
    // Pulse 1: $4000-$4003
    if (addr >= 0x4000 && addr <= 0x4003) {
        write_pulse(&apu->pulse[0], addr - 0x4000, data);
    }
    // Pulse 2: $4004-$4007
    else if (addr >= 0x4004 && addr <= 0x4007) {
        write_pulse(&apu->pulse[1], addr - 0x4004, data);
    }
    // Triangle: $4008-$400B
    else if (addr == 0x4008) {
        apu->triangle.control_flag = (data >> 7) & 1;
        apu->triangle.linear_counter_reload_value = data & 0x7F;
    } else if (addr == 0x400A) {
        apu->triangle.timer_period =
            (apu->triangle.timer_period & 0x700) | data;
    } else if (addr == 0x400B) {
        apu->triangle.timer_period =
            (apu->triangle.timer_period & 0xFF) | ((uint16_t)(data & 7) << 8);
        if (apu->triangle.enabled) {
            apu->triangle.length_counter = LENGTH_TABLE[data >> 3];
        }
        apu->triangle.linear_counter_reload_flag = true;
    }
    // Noise: $400C-$400F
    else if (addr == 0x400C) {
        apu->noise.envelope_loop = (data >> 5) & 1;
        apu->noise.constant_volume = (data >> 4) & 1;
        apu->noise.envelope_volume = data & 0x0F;
    } else if (addr == 0x400E) {
        apu->noise.mode = (data >> 7) & 1;
        apu->noise.timer_period = NOISE_PERIOD_TABLE[data & 0x0F];
    } else if (addr == 0x400F) {
        if (apu->noise.enabled) {
            apu->noise.length_counter = LENGTH_TABLE[data >> 3];
        }
        apu->noise.envelope_start = true;
    }
    // DMC: $4010-$4013
    else if (addr == 0x4011) {
        apu->dmc.output_level = data & 0x7F;
    }
    // Status: $4015
    else if (addr == 0x4015) {
        apu->pulse[0].enabled = data & 1;
        apu->pulse[1].enabled = (data >> 1) & 1;
        apu->triangle.enabled = (data >> 2) & 1;
        apu->noise.enabled = (data >> 3) & 1;

        if (!apu->pulse[0].enabled)
            apu->pulse[0].length_counter = 0;
        if (!apu->pulse[1].enabled)
            apu->pulse[1].length_counter = 0;
        if (!apu->triangle.enabled)
            apu->triangle.length_counter = 0;
        if (!apu->noise.enabled)
            apu->noise.length_counter = 0;
    }
    // Frame counter: $4017
    else if (addr == 0x4017) {
        apu->frame_mode = (data >> 7) & 1;
        apu->frame_irq_inhibit = (data >> 6) & 1;
        if (apu->frame_irq_inhibit) {
            apu->frame_irq = false;
            apu->irq_pending = false;
        }
        apu->frame_counter = 0;
        if (apu->frame_mode == 1) {
            // 5-step mode: immediately clock half frame
            clock_half_frame(apu);
        }
    }
}

uint8_t apu_read_register(APU* apu, uint16_t addr) {
    if (addr != 0x4015)
        return 0;

    uint8_t status = 0;
    if (apu->pulse[0].length_counter > 0)
        status |= 0x01;
    if (apu->pulse[1].length_counter > 0)
        status |= 0x02;
    if (apu->triangle.length_counter > 0)
        status |= 0x04;
    if (apu->noise.length_counter > 0)
        status |= 0x08;
    if (apu->frame_irq)
        status |= 0x40;

    apu->frame_irq = false;
    apu->irq_pending = false;
    return status;
}

void apu_audio_callback(void* userdata, uint8_t* stream, int len) {
    APU* apu = (APU*)userdata;
    float* out = (float*)stream;
    int samples = len / (int)sizeof(float);

    for (int i = 0; i < samples; i++) {
        if (apu->buffer_read != apu->buffer_write) {
            out[i] = apu->buffer[apu->buffer_read];
            apu->buffer_read = (apu->buffer_read + 1) % APU_BUFFER_SIZE;
        } else {
            out[i] = 0.0f;
        }
    }
}
