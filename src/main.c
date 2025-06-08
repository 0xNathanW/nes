#include <SDL2/SDL.h>
#include "ines.h"

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

int main(int argc, char *argv[]) { 
    INES_Cart* cart = load_cart("test_roms/dk.nes");
    if (!cart) {
        printf("error: could not load cart\n");
        return 1;
    }
    print_cart_info(cart);
    free_cart_memory(cart);
    return 0;     
}
