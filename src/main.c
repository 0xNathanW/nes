#include "nes.h"
#include <SDL.h>
#include <stdio.h>

#define WINDOW_SCALE 3
#define WINDOW_WIDTH (NES_WIDTH * WINDOW_SCALE)
#define WINDOW_HEIGHT (NES_HEIGHT * WINDOW_SCALE)

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
    if (argc < 2) {
        printf("usage: nes <rom.nes>\n");
        return 1;
    }

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("error: SDL initialisation failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window* window =
        SDL_CreateWindow("NES", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                         WINDOW_WIDTH, WINDOW_HEIGHT, 0);
    if (!window) {
        printf("error: SDL window creation failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(
        window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
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

    if (!nes_load_cartridge(nes, argv[1])) {
        printf("error: could not load cartridge: %s\n", argv[1]);
        nes_destroy(nes);
        SDL_DestroyTexture(texture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    print_cart_info(nes->cartridge);

    uint32_t pixels[NES_WIDTH * NES_HEIGHT];
    bool running = true;

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
            if (event.type == SDL_KEYDOWN &&
                event.key.keysym.sym == SDLK_ESCAPE) {
                running = false;
            }
            if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
                bool pressed = (event.type == SDL_KEYDOWN);
                uint8_t btn = 0;
                switch (event.key.keysym.sym) {
                case SDLK_z:      btn = BUTTON_A;      break; // A
                case SDLK_x:      btn = BUTTON_B;      break; // B
                case SDLK_RSHIFT: btn = BUTTON_SELECT;  break; // Select
                case SDLK_RETURN: btn = BUTTON_START;   break; // Start
                case SDLK_UP:     btn = BUTTON_UP;      break;
                case SDLK_DOWN:   btn = BUTTON_DOWN;    break;
                case SDLK_LEFT:   btn = BUTTON_LEFT;    break;
                case SDLK_RIGHT:  btn = BUTTON_RIGHT;   break;
                default: break;
                }
                if (btn) {
                    if (pressed)
                        nes->bus.controller_state[0] |= btn;
                    else
                        nes->bus.controller_state[0] &= ~btn;
                }
            }
        }

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
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
    }

    nes_destroy(nes);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
