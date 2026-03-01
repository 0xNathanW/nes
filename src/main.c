#include "apu.h"
#include "nes.h"
#include <SDL.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#define DEFAULT_SCALE 3

#define OVERSCAN 8 // Pixels cropped from each edge

typedef struct {
    const char* rom_path;
    int scale;
    bool overscan;
} Options;

static void print_usage(const char* prog) {
    printf("usage: %s [options] <rom.nes>\n\n", prog);
    printf("options:\n");
    printf("  -s, --scale <n>   Window scale factor (default: %d)\n",
           DEFAULT_SCALE);
    printf("  -o, --overscan    Crop %d pixels from each edge\n", OVERSCAN);
    printf("  -h, --help        Show this help message\n");
}

static bool parse_options(int argc, char* argv[], Options* opts) {
    opts->rom_path = NULL;
    opts->scale = DEFAULT_SCALE;
    opts->overscan = false;

    // clang-format off
    static struct option long_options[] = {
        {"scale",    required_argument, NULL, 's'},
        {"overscan", no_argument,       NULL, 'o'},
        {"help",     no_argument,       NULL, 'h'},
        {NULL,       0,                 NULL,  0 },
    };
    // clang-format on

    int opt;
    while ((opt = getopt_long(argc, argv, "s:oh", long_options, NULL)) != -1) {
        switch (opt) {
        case 's':
            opts->scale = atoi(optarg);
            if (opts->scale < 1 || opts->scale > 8) {
                fprintf(stderr, "error: scale must be between 1 and 8\n");
                return false;
            }
            break;
        case 'o':
            opts->overscan = true;
            break;
        case 'h':
            print_usage(argv[0]);
            exit(0);
        default:
            print_usage(argv[0]);
            return false;
        }
    }

    if (optind >= argc) {
        fprintf(stderr, "error: no ROM file specified\n\n");
        print_usage(argv[0]);
        return false;
    }

    opts->rom_path = argv[optind];
    return true;
}

// clang-format off
static const uint32_t NES_PALETTE[64] = {
    0x666666, 0x002A88, 0x1412A7, 0x3B00A4,
    0x5C007E, 0x6E0040, 0x6C0600, 0x561D00,
    0x333500, 0x0B4800, 0x005200, 0x004F08,
    0x00404D, 0x000000, 0x000000, 0x000000,
    0xADADAD, 0x155FD9, 0x4240FF, 0x7527FE,
    0xA01ACC, 0xB71E7B, 0xB53120, 0x994E00,
    0x6B6D00, 0x388700, 0x0C9300, 0x008F32,
    0x007C8D, 0x000000, 0x000000, 0x000000,
    0xFFFEFF, 0x64B0FF, 0x9290FF, 0xC676FF,
    0xF36AFF, 0xFE6ECC, 0xFE8170, 0xEA9E22,
    0xBCBE00, 0x88D800, 0x5CE430, 0x45E082,
    0x48CDDE, 0x4F4F4F, 0x000000, 0x000000,
    0xFFFEFF, 0xC0DFFF, 0xD3D2FF, 0xE8C8FF,
    0xFBC2FF, 0xFEC4EA, 0xFECCC5, 0xF7D8A5,
    0xE4E594, 0xCFEF96, 0xBDF4AB, 0xB3F3CC,
    0xB5EBF2, 0xB8B8B8, 0x000000, 0x000000,
};
// clang-format on

int main(int argc, char* argv[]) {
    Options opts;
    if (!parse_options(argc, argv, &opts)) {
        return 1;
    }

    int view_w = opts.overscan ? NES_WIDTH - 2 * OVERSCAN : NES_WIDTH;
    int view_h = opts.overscan ? NES_HEIGHT - 2 * OVERSCAN : NES_HEIGHT;
    int window_width = view_w * opts.scale;
    int window_height = view_h * opts.scale;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        printf("error: SDL initialisation failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window* window =
        SDL_CreateWindow("NES", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                         window_width, window_height, 0);
    if (!window) {
        printf("error: SDL window creation failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer =
        SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        printf("error: renderer creation failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_Texture* texture =
        SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                          SDL_TEXTUREACCESS_STREAMING, NES_WIDTH, NES_HEIGHT);
    if (!texture) {
        printf("error: texture creation failed: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    NES* nes = nes_create();
    if (!nes) {
        printf("error: could not create NES\n");
        SDL_DestroyTexture(texture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    if (!nes_load_cartridge(nes, opts.rom_path)) {
        printf("error: could not load cartridge: %s\n", opts.rom_path);
        nes_destroy(nes);
        SDL_DestroyTexture(texture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    print_cart_info(nes->cartridge);

    // Open SDL audio device
    SDL_AudioDeviceID audio_dev = 0;
    {
        SDL_AudioSpec want;
        SDL_zero(want);
        want.freq = APU_SAMPLE_RATE;
        want.format = AUDIO_F32SYS;
        want.channels = 1;
        want.samples = 1024;
        want.callback = apu_audio_callback;
        want.userdata = &nes->apu;
        audio_dev = SDL_OpenAudioDevice(NULL, 0, &want, NULL, 0);
        if (audio_dev == 0) {
            printf("warning: could not open audio: %s\n", SDL_GetError());
        } else {
            SDL_PauseAudioDevice(audio_dev, 0);
        }
    }

    uint32_t pixels[NES_WIDTH * NES_HEIGHT];
    bool running = true;
    bool muted = false;

    const double NES_FPS = 60.0988;
    const double frame_target_s = 1.0 / NES_FPS;
    uint64_t perf_freq = SDL_GetPerformanceFrequency();
    uint64_t frame_start = SDL_GetPerformanceCounter();

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
            if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    running = false;
                }
                if (event.key.keysym.sym == SDLK_m && audio_dev) {
                    muted = !muted;
                    SDL_PauseAudioDevice(audio_dev, muted);
                }
            }
        }

        const uint8_t* keys = SDL_GetKeyboardState(NULL);
        uint8_t pad = 0;
        if (keys[SDL_SCANCODE_Z])      pad |= BUTTON_A;
        if (keys[SDL_SCANCODE_X])      pad |= BUTTON_B;
        if (keys[SDL_SCANCODE_RSHIFT]) pad |= BUTTON_SELECT;
        if (keys[SDL_SCANCODE_RETURN]) pad |= BUTTON_START;
        if (keys[SDL_SCANCODE_UP])     pad |= BUTTON_UP;
        if (keys[SDL_SCANCODE_DOWN])   pad |= BUTTON_DOWN;
        if (keys[SDL_SCANCODE_LEFT])   pad |= BUTTON_LEFT;
        if (keys[SDL_SCANCODE_RIGHT])  pad |= BUTTON_RIGHT;
        nes->bus.controller_state[0] = pad;

        // Run one frame
        nes->ppu.frame_complete = false;
        while (!nes->ppu.frame_complete) {
            nes_step(nes);
        }

        // Convert framebuffer palette indices to ARGB
        for (int i = 0; i < NES_WIDTH * NES_HEIGHT; i++) {
            uint8_t pal_index = nes->ppu.framebuf[i] & 0x3F;
            pixels[i] = 0xFF000000 | NES_PALETTE[pal_index];
        }

        SDL_UpdateTexture(texture, NULL, pixels, NES_WIDTH * sizeof(uint32_t));
        SDL_RenderClear(renderer);
        if (opts.overscan) {
            SDL_Rect src = {OVERSCAN, OVERSCAN, view_w, view_h};
            SDL_RenderCopy(renderer, texture, &src, NULL);
        } else {
            SDL_RenderCopy(renderer, texture, NULL, NULL);
        }
        SDL_RenderPresent(renderer);

        // Wait until frame deadline
        uint64_t now = SDL_GetPerformanceCounter();
        double elapsed = (double)(now - frame_start) / (double)perf_freq;
        double remaining = frame_target_s - elapsed;
        if (remaining > 0.0015) {
            SDL_Delay((uint32_t)((remaining - 0.001) * 1000.0));
        }
        while ((double)(SDL_GetPerformanceCounter() - frame_start) /
                   (double)perf_freq <
               frame_target_s) {
            // busy-wait for sub-ms precision
        }
        frame_start = SDL_GetPerformanceCounter();
    }

    if (audio_dev)
        SDL_CloseAudioDevice(audio_dev);
    nes_destroy(nes);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
