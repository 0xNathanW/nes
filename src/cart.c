#include "cart.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

static bool is_header_valid(struct CartHeader* header) {
    char expected_signature[4] = {0x4E, 0x45, 0x53, 0x1A};
    if (memcmp(expected_signature, header->signature, 4)) {
        printf("error: not a valid iNES ROM (bad signature)\n");
        return false;
    }

    if ((header->flags7 & 0x0C) == 0x08) {
        printf("info: iNES 2.0 detected, parsing as iNES 1.0\n");
    }

    // Don't support ines 2.0 atm.
    if ((header->flags8 != 0) && (header->flags9 != 0) &&
        (header->flags10 != 0)) {
        printf("error: this cart is not supported\n");
        return false;
    }

    return true;
}

Cartridge* load_cart(const char* path) {

    FILE* file = fopen(path, "rb");
    if (!file) {
        perror("error: could not open cartridge");
        return NULL;
    }

    Cartridge* cart = malloc(sizeof(Cartridge));
    if (!cart) {
        printf("error: unable to allocate memory for cart\n");
        fclose(file);
        return NULL;
    }

    // Initialise structure to zeros.
    memset(cart, 0, sizeof(Cartridge));

    if (fread(&cart->header, sizeof(struct CartHeader), 1, file) != 1) {
        printf("error: could not read cart header\n");
        free_cart(cart);
        fclose(file);
        return NULL;
    }

    if (!is_header_valid(&cart->header)) {
        free_cart(cart);
        fclose(file);
        return NULL;
    }

    cart->prg_rom_size = cart->header.prg_rom_size;
    cart->chr_rom_size = cart->header.chr_rom_size;
    cart->has_trainer = (cart->header.flags6 & 0x04) != 0;
    cart->nametable_arrangement = (cart->header.flags6 & 0x01);
    cart->is_battery_backed = (cart->header.flags6 & 0x02) != 0;
    cart->mapper_number =
        (cart->header.flags7 & 0xF0) | (cart->header.flags6 >> 4);

    // Read the trainer if exists.
    if (cart->has_trainer) {
        cart->trainer = malloc(512);
        if (!cart->trainer) {
            printf("error: unable to allocate memory for trainer\n");
            free_cart(cart);
            fclose(file);
            return NULL;
        }
        if (fread(cart->trainer, 512, 1, file) != 1) {
            printf("error: unable to read trainer data\n");
            free_cart(cart);
            fclose(file);
            return NULL;
        }
    }

    // Read cart code.
    if (cart->prg_rom_size > 0) {
        size_t prg_bytes = cart->prg_rom_size * PRG_BLOCK;
        cart->prg_rom = malloc(prg_bytes);
        if (!cart->prg_rom) {
            printf("error: unable to allocate memory for program code\n");
            free_cart(cart);
            fclose(file);
            return NULL;
        }
        if (fread(cart->prg_rom, prg_bytes, 1, file) != 1) {
            printf("error: unable to read program code\n");
            free_cart(cart);
            fclose(file);
            return NULL;
        }
    }

    // Read graphics data.
    if (cart->chr_rom_size > 0) {
        size_t chr_bytes = cart->chr_rom_size * CHR_BLOCK;
        cart->chr_rom = malloc(chr_bytes);
        if (!cart->chr_rom) {
            printf("error: unable to allocate memory for graphics data\n");
            free_cart(cart);
            fclose(file);
            return NULL;
        }
        if (fread(cart->chr_rom, chr_bytes, 1, file) != 1) {
            printf("error: unable to read graphics data\n");
            free_cart(cart);
            fclose(file);
            return NULL;
        }
    }

    fclose(file);

    // Initialize mapper
    switch (cart->mapper_number) {
    case 0:
        mapper0_init(cart);
        break;
    case 1:
        mapper1_init(cart);
        break;
    default:
        printf("error: unsupported mapper %d\n", cart->mapper_number);
        free_cart(cart);
        return NULL;
    }

    return cart;
}

void free_cart(Cartridge* cart) {
    if (!cart) {
        return;
    }
    if (cart->trainer) {
        free(cart->trainer);
    }
    if (cart->prg_rom) {
        free(cart->prg_rom);
    }
    if (cart->chr_rom) {
        free(cart->chr_rom);
    }
    if (cart->mapper_data) {
        free(cart->mapper_data);
    }
    free(cart);
}

void print_cart_info(Cartridge* cart) {
    printf("\nCart Info:\n");
    printf("PRG ROM size: %d KB\n", cart->prg_rom_size * 16);
    printf("CHR ROM size: %d KB\n", cart->chr_rom_size * 8);
    printf("Mapper number: %d\n", cart->mapper_number);
    printf("Mirroring: %s\n",
           cart->nametable_arrangement ? "vertical" : "horizontal");
    if (cart->is_battery_backed) {
        printf("Battery backed memory present\n");
    }
    if (cart->has_trainer) {
        printf("512 byte trainer present\n");
    }
    printf("\n");
}

uint8_t cart_read_byte(Cartridge* cart, uint16_t addr) {
    return cart->mapper.read_prg(cart, addr);
}

void cart_write_byte(Cartridge* cart, uint16_t addr, uint8_t data) {
    cart->mapper.write_prg(cart, addr, data);
}

uint8_t cart_read_chr(Cartridge* cart, uint16_t addr) {
    return cart->mapper.read_chr(cart, addr);
}

void cart_write_chr(Cartridge* cart, uint16_t addr, uint8_t data) {
    cart->mapper.write_chr(cart, addr, data);
}
