#include "mmu.h"
#include <stdio.h>
#include <stdlib.h>

#define Z64_IDENTIFIER 0x80371240
#define N64_IDENTIFIER 0x40123780
#define V64_IDENTIFIER 0x37804012

int mmuInit(FILE* romf) {
	// Get File Size
	fseek(romf, 0L, SEEK_END);
	rom_size = ftell(romf);
	fseek(romf, 0L, SEEK_SET);

	// Load ROM into memory
	rom = malloc(rom_size);
	fread(rom, rom_size, 1, romf);
	byteswapRom();

	printf(" [ INF ] Loaded ROM (%d bytes)\n\n", rom_size);

    // Initialize SP DMEM
    SPDmem = malloc(0x1000);
    if (SPDmem == NULL)
        return -1;

    // Initialize RI Registers
    RIreg = malloc(32);
    if (RIreg == NULL)
        return -1;
    for (int i = 0; i < 32; i++)
        RIreg[i] = 0;

	return 0;
}

u8 readu8(u32 vaddr) {
    if (vaddr < (u64)0x80000000)           // KUSEG
        return 0xFF;
    else if (vaddr < (u64)0xA0000000)      // KSEG0
        return readPhys(vaddr & 0x1FFFFFFF);
    else if (vaddr < (u64)0xC0000000)      // KSEG1
        return readPhys(vaddr & 0x1FFFFFFF);
    else if (vaddr < (u64)0xE0000000)      // KSSEG
        return 0xFF;
    else                              // KSEG3
        return 0xFF;
}

u16 readu16(u32 vaddr) {
    return (((u16)readu8(vaddr)) << 8) | readu8(vaddr + 1);
}

u32 readu32(u32 vaddr) {
    return (((u32)readu16(vaddr)) << 16) | readu16(vaddr + 2);
}

u64 readu64(u32 vaddr) {
    return (((u64)readu32(vaddr)) << 32) | readu32(vaddr + 4);
}

i8 readi8(u32 vaddr) {
    return (i8)readu8(vaddr);
}

i16 readi16(u32 vaddr) {
    return (i16)readu16(vaddr);
}

i32 readi32(u32 vaddr) {
    return (i32)readu32(vaddr);
}

i64 readi64(u32 vaddr) {
    return (i64)readu64(vaddr);
}

void writeu8(u32 vaddr, u8 val) {
    if (vaddr < 0x80000000)           // KUSEG
        return;
    else if (vaddr < 0xA0000000)      // KSEG0
        writePhys(vaddr & 0x1FFFFFFF, val);
    else if (vaddr < 0xC0000000)      // KSEG1
        writePhys(vaddr & 0x1FFFFFFF, val);
    else if (vaddr < 0xC0000000)      // KSSEG
        return;
    else                              // KSEG3
        return;
}

void writeu16(u32 vaddr, u16 val) {
    writeu8(vaddr, (val >> 8) & 0xFF);
    writeu8(vaddr + 1, val & 0xFF);
}

void writeu32(u32 vaddr, u32 val) {
    writeu16(vaddr, (val >> 16) & 0xFFFF);
    writeu16(vaddr + 2, val & 0xFFFF);
}

void writeu64(u32 vaddr, u64 val) {
    writeu32(vaddr, (val >> 32) & 0xFFFFFFFF);
    writeu32(vaddr + 4, val & 0xFFFFFFFF);
}

u8 readPhys(u32 paddr) {
    if (paddr < 0x00400000) {
        // RDRAM - built in
    }
    else if (paddr < 0x00800000) {
        // RDRAM - expansion pak
    }
    else if (paddr < 0x03F00000) {
        // Unused
    }
    else if (paddr < 0x04000000) {
        // RDRAM Registers
    }
    else if (paddr < 0x04001000) {
        // SP DMEM
        return SPDmem[paddr & 0xFFF];
    }
    else if (paddr < 0x04002000) {
        // SP IMEM
    }
    else if (paddr < 0x04040000) {
        // Unused
    }
    else if (paddr < 0x04100000) {
        // SP Registers
    }
    else if (paddr < 0x04200000) {
        // DP Command Registers
    }
    else if (paddr < 0x04300000) {
        // DP Span Registers
    }
    else if (paddr < 0x04400000) {
        // MIPS Interface
    }
    else if (paddr < 0x04500000) {
        // Video Interface
    }
    else if (paddr < 0x04600000) {
        // Audio Interface
    }
    else if (paddr < 0x04700000) {
        // Peripheral Interface
    }
    else if (paddr < 0x04800000) {
        // RDRAM Interface
        if (paddr > 0x0470001F)
            return 0xFF;
        else
            return RIreg[(paddr >> 2) & 0x1F];
    }
    else if (paddr < 0x04900000) {
        // Serial Interface
    }
    else if (paddr < 0x05000000) {
        // Unused
    }
    else if (paddr < 0x06000000) {
        // Cartridge Domain 2 Address 1
    }
    else if (paddr < 0x08000000) {
        // Cartridge Domain 1 Address 1
    }
    else if (paddr < 0x10000000) {
        // Cartridge Domain 2 Address 2
    }
    else if (paddr < 0x1FC00000) {
        // Cartridge Domain 1 Address 2
        return rom[paddr - 0x10000000];
    }
    else if (paddr < 0x1FC007C0) {
        // PIF Boot ROM
    }
    else if (paddr < 0x1FC00800) {
        // PIF RAM
    }
    else if (paddr < 0x1FD00000) {
        // PIF RAM
    }
    else if (paddr < 0x80000000) {
        // Cartridge Domain 1 Address 3
    }
    else {
        // Unknown
    }
    return 0xFF;
}

void writePhys(u32 paddr, u8 val) {
    if (paddr < 0x00400000) {
        // RDRAM - built in
    }
    else if (paddr < 0x00800000) {
        // RDRAM - expansion pak
    }
    else if (paddr < 0x03F00000) {
        // Unused
    }
    else if (paddr < 0x04000000) {
        // RDRAM Registers
    }
    else if (paddr < 0x04001000) {
        // SP DMEM
        SPDmem[paddr & 0xFFF] = val;
    }
    else if (paddr < 0x04002000) {
        // SP IMEM
    }
    else if (paddr < 0x04040000) {
        // Unused
    }
    else if (paddr < 0x04100000) {
        // SP Registers
    }
    else if (paddr < 0x04200000) {
        // DP Command Registers
    }
    else if (paddr < 0x04300000) {
        // DP Span Registers
    }
    else if (paddr < 0x04400000) {
        // MIPS Interface
    }
    else if (paddr < 0x04500000) {
        // Video Interface
    }
    else if (paddr < 0x04600000) {
        // Audio Interface
    }
    else if (paddr < 0x04700000) {
        // Peripheral Interface
    }
    else if (paddr < 0x04800000) {
        // RDRAM Interface
        if (paddr > 0x0470001F)
            return;
        RIreg[(paddr >> 2) & 0x1F] = val;
    }
    else if (paddr < 0x04900000) {
        // Serial Interface
    }
    else if (paddr < 0x05000000) {
        // Unused
    }
    else if (paddr < 0x06000000) {
        // Cartridge Domain 2 Address 1
    }
    else if (paddr < 0x08000000) {
        // Cartridge Domain 1 Address 1
    }
    else if (paddr < 0x10000000) {
        // Cartridge Domain 2 Address 2
    }
    else if (paddr < 0x1FC00000) {
        // Cartridge Domain 1 Address 2
    }
    else if (paddr < 0x1FC007C0) {
        // PIF Boot ROM
    }
    else if (paddr < 0x1FC00800) {
        // PIF RAM
    }
    else if (paddr < 0x1FD00000) {
        // PIF RAM
    }
    else if (paddr < 0x80000000) {
        // Cartridge Domain 1 Address 3
    }
    else {
        // Unknown
    }
}

void byteswapRom() {
    int ident = (rom[3] & 0x000000ff) +
        ((rom[2] << 8) & 0x0000ff00) +
        ((rom[1] << 16) & 0x00ff0000) +
        ((rom[0] << 24) & 0xff000000);

    switch (ident) {
    case Z64_IDENTIFIER:
        printf(" [ INF ] Z64 formatted file, no byteswapping.\n");
        return;
    case N64_IDENTIFIER:
        printf(" [ INF ] Byteswapping N64 formatted file...\n");
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
        printf(" [ INF ] Byteswapping V64 formatted file...\n");
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
        printf(" [ INF ] Unknown ROM Format %lX! Issues may occur.\n", ident);
    }
}