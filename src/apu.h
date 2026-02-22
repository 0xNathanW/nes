#ifndef APU_H
#define APU_H

#include <stdbool.h>
#include <stdint.h>

#define APU_SAMPLE_RATE 44100
#define APU_BUFFER_SIZE 4096

typedef struct {
    // Envelope
    bool envelope_start;
    uint8_t envelope_divider;
    uint8_t envelope_decay;
    uint8_t envelope_volume;
    bool envelope_loop;
    bool constant_volume;

    // Sweep
    bool sweep_enabled;
    uint8_t sweep_period;
    bool sweep_negate;
    uint8_t sweep_shift;
    bool sweep_reload;
    uint8_t sweep_divider;

    // Timer
    uint16_t timer_period;
    uint16_t timer_value;

    // Duty
    uint8_t duty;
    uint8_t duty_pos;

    // Length counter
    uint8_t length_counter;

    bool enabled;
} PulseChannel;

typedef struct {
    // Linear counter
    bool linear_counter_reload_flag;
    uint8_t linear_counter_reload_value;
    uint8_t linear_counter;
    bool control_flag;

    // Timer
    uint16_t timer_period;
    uint16_t timer_value;

    // Sequencer
    uint8_t seq_pos;

    // Length counter
    uint8_t length_counter;

    bool enabled;
} TriangleChannel;

typedef struct {
    // Envelope
    bool envelope_start;
    uint8_t envelope_divider;
    uint8_t envelope_decay;
    uint8_t envelope_volume;
    bool envelope_loop;
    bool constant_volume;

    // Timer
    uint16_t timer_period;
    uint16_t timer_value;

    // LFSR
    uint16_t shift_register;
    bool mode;

    // Length counter
    uint8_t length_counter;

    bool enabled;
} NoiseChannel;

typedef struct {
    uint8_t output_level;
} DMCChannel;

typedef struct APU {
    PulseChannel pulse[2];
    TriangleChannel triangle;
    NoiseChannel noise;
    DMCChannel dmc;

    // Frame counter
    uint32_t frame_counter;
    uint8_t frame_mode;
    bool frame_irq_inhibit;
    bool frame_irq;
    bool irq_pending;

    // Cycle tracking
    bool odd_cycle;

    // Audio output ring buffer
    float buffer[APU_BUFFER_SIZE];
    volatile uint32_t buffer_read;
    volatile uint32_t buffer_write;

    // Downsampling
    double sample_accumulator;
    int sample_count;
    double cycles_per_sample;
    double sample_period;
} APU;

void apu_init(APU* apu);
void apu_step(APU* apu);
void apu_write_register(APU* apu, uint16_t addr, uint8_t data);
uint8_t apu_read_register(APU* apu, uint16_t addr);
void apu_audio_callback(void* userdata, uint8_t* stream, int len);

#endif
