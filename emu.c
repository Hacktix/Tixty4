#include "emu.h"
#include "cpu.h"
#include <stdio.h>
#include <stdlib.h>

#define Z64_IDENTIFIER 0x80371240
#define N64_IDENTIFIER 0x40123780
#define V64_IDENTIFIER 0x37804012

void emuStart() {
    cpuInit();
}

void loadRom(FILE* romf) {
	// Get File Size
	fseek(romf, 0L, SEEK_END);
	rom_size = ftell(romf);
	fseek(romf, 0L, SEEK_SET);

	// Load ROM into memory
	rom = malloc(rom_size);
	fread(rom, rom_size, 1, romf);
	byteswapRom();

    printf("Loaded ROM (%d bytes)", rom_size);
}

void byteswapRom() {
    int ident = (rom[3] & 0x000000ff) +
        ((rom[2] << 8) & 0x0000ff00) +
        ((rom[1] << 16) & 0x00ff0000) +
        ((rom[0] << 24) & 0xff000000);

    switch (ident) {
        case Z64_IDENTIFIER:
            printf("Z64 formatted file, no byteswapping.\n");
            return;
        case N64_IDENTIFIER:
            printf("Byteswapping N64 formatted file...\n");
            for (int i = 0; i < rom_size / 4; i++) {
                int w;
                memcpy(&w, rom + (i * 4), 4);
                int swapped =
                    (0xFF000000 & (w << 24)) |
                    (0x00FF0000 & (w << 8)) |
                    (0x0000FF00 & (w >> 8)) |
                    (0x000000FF & (w >> 24));
                memcpy(rom + (i * 4), &swapped, 4);
            }
            break;
        case V64_IDENTIFIER:
            printf("Byteswapping V64 formatted file...\n");
            for (int i = 0; i < rom_size / 4; i++) {
                int w;
                memcpy(&w, rom + (i * 4), 4);
                int swapped =
                    (0xFF000000 & (w << 8)) |
                    (0x00FF0000 & (w >> 8)) |
                    (0x0000FF00 & (w << 8)) |
                    (0x000000FF & (w >> 8));
                memcpy(rom + (i * 4), &swapped, 4);
            }
            return;
        default:
            printf("Unknown ROM Format %lx! Issues may occur.\n", ident);
    }
}