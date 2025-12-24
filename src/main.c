#include <SDL2/SDL.h>
#include "cart.h"
#include "nes.h"

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

int main(int argc, char *argv[]) { 
    Cartridge* cart = load_cart("test_roms/dk.nes");
    if (!cart) {
        printf("error: could not load cart\n");
        return 1;
    }
    print_cart_info(cart);
    
    NES* nes = nes_create();
    if (!nes) {
        printf("error: could not create NES\n");
        free_cart(cart);
        return 1;
    }

    while (1) {
    }

    nes_destroy(nes);
    free_cart(cart);
    return 0;     
}
