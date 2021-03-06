#include "mmu.h"
#include "emu.h"
#include "cpu.h"
#include <stdio.h>
#include <stdlib.h>

#define Z64_IDENTIFIER 0x80371240
#define N64_IDENTIFIER 0x40123780
#define V64_IDENTIFIER 0x37804012

#define PI_BASE_REG      0x04600000
#define PI_DRAM_ADDR_REG 0x0
#define PI_CART_ADDR_REG 0x4
#define PI_RD_LEN_REG    0x8
#define PI_WR_LEN_REG    0xC

u8 mi_intr_mask = 0;
u8 mi_intr_reg = 0;

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

    // Initialize RDRAM
    RDRAM = malloc(0x800000);
    if (RDRAM == NULL)
        return -1;

    // Initialize SP DMEM
    SPDmem = malloc(0x1000);
    if (SPDmem == NULL)
        return -1;

    // Initialize SP IMEM
    SPImem = malloc(0x1000);
    if (SPImem == NULL)
        return -1;

    // Initialize RI Registers
    RIreg = malloc(0x20);
    if (RIreg == NULL)
        return -1;
    for (int i = 0; i < 0x20; i++)
        RIreg[i] = 0;

    // Initialize PI Registers
    PIreg = malloc(0x34);
    if (PIreg == NULL)
        return -1;
    for (int i = 0; i < 0x34; i++)
        PIreg[i] = 0;

    // Initialize RDRAM Registers
    RDRAMreg = malloc(0x28);
    if (RDRAMreg == NULL)
        return -1;
    for (int i = 0; i < 0x28; i++)
        RDRAMreg[i] = 0;

    // Initialize MI Registers
    MIreg = malloc(0x10);
    if (MIreg == NULL)
        return -1;
    for (int i = 0; i < 0x10; i++)
        MIreg[i] = 0;

    // Initialize SP Registers
    SPreg = malloc(0x40007);
    if (SPreg == NULL)
        return -1;
    for (int i = 0; i < 0x40007; i++)
        SPreg[i] = 0;

    // Initialize SI Registers
    SIreg = malloc(0x1C);
    if (SIreg == NULL)
        return -1;
    for (int i = 0; i < 0x1C; i++)
        SIreg[i] = 0;

    // Initialize AI Registers
    AIreg = malloc(0x18);
    if (AIreg == NULL)
        return -1;
    for (int i = 0; i < 0x18; i++)
        AIreg[i] = 0;

    // Initialize PIF RAM
    PIFram = malloc(0x40);
    if (PIFram == NULL)
        return -1;
    for (int i = 0; i < 0x40; i++)
        PIFram[i] = 0;

    // Initialize VI Registers
    VIreg = malloc(0x38);
    if (VIreg == NULL)
        return -1;
    for (int i = 0; i < 0x38; i++)
        VIreg[i] = 0;

	return 0;
}

u8 readu8(u32 vaddr) {
    if (vaddr < (u64)0x80000000) {           // KUSEG
        printf(" [ WRN ] Unknown Read from KUSEG (0x%08X) at PC=0x%016llX. [Press Enter to continue anyway]", vaddr, pc-4);
        getchar();
        return 0xFF;
    }
    else if (vaddr < (u64)0xA0000000) {      // KSEG0
        return readPhys(vaddr & 0x1FFFFFFF);
    }
    else if (vaddr < (u64)0xC0000000) {      // KSEG1
        return readPhys(vaddr & 0x1FFFFFFF);
    }
    else if (vaddr < (u64)0xE0000000) {      // KSSEG
        printf(" [ WRN ] Unknown Read from KSSEG (0x%08X) at PC=0x%016llX. [Press Enter to continue anyway]", vaddr, pc-4);
        getchar();
        return 0xFF;
    }
    else {                              // KSEG3
        printf(" [ WRN ] Unknown Read from KSEG3 (0x%08X) at PC=0x%016llX. [Press Enter to continue anyway]", vaddr, pc-4);
        getchar();
        return 0xFF;
    }
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
        return RDRAM[paddr];
    }
    else if (paddr < 0x00800000) {
        // RDRAM - expansion pak
        return RDRAM[paddr];
    }
    else if (paddr < 0x03F00000) {
        // Unused
    }
    else if (paddr < 0x04000000) {
        // RDRAM Registers
        if (paddr > 0x03F00027)
            return 0xFF;
        else
            return RDRAMreg[paddr & 0x3F];
    }
    else if (paddr < 0x04001000) {
        // SP DMEM
        return SPDmem[paddr & 0xFFF];
    }
    else if (paddr < 0x04002000) {
        // SP IMEM
        return SPImem[paddr & 0xFFF];
    }
    else if (paddr < 0x04040000) {
        // Unused
    }
    else if (paddr < 0x04100000) {
        // SP Registers
        if (paddr > 0x04080008)
            return 0xFF;
        if (paddr > 0x0404001F && paddr < 0x04080000)
            return 0xFF;
        return SPreg[paddr & 0x3FFFF];
    }
    else if (paddr < 0x04200000) {
        // DP Command Registers
    }
    else if (paddr < 0x04300000) {
        // DP Span Registers
    }
    else if (paddr < 0x04400000) {
        // MIPS Interface
        if (paddr > 0x0430000F)
            return 0xFF;

        switch (paddr) {
        case 0x04300008:
        case 0x04300009:
        case 0x0430000A:
            return 0;
        case 0x0430000B:
            return mi_intr_reg;

        case 0x0430000C:
        case 0x0430000D:
        case 0x0430000E:
            return 0;
        case 0x0430000F:
            return mi_intr_mask;
        }

        return MIreg[paddr & 0xF];
    }
    else if (paddr < 0x04500000) {
        // Video Interface
        if (paddr > 0x04400037)
            return 0xFF;
        return VIreg[paddr & 0x3F];
    }
    else if (paddr < 0x04600000) {
        // Audio Interface
        if (paddr > 0x04500017)
            return 0xFF;
        return AIreg[paddr & 0x1F];
    }
    else if (paddr < 0x04700000) {
        // Peripheral Interface
        if (paddr > 0x04600034)
            return 0xFF;
        return PIreg[paddr & 0x3F];
    }
    else if (paddr < 0x04800000) {
        // RDRAM Interface
        if (paddr > 0x0470001F)
            return 0xFF;
        else
            return RIreg[paddr & 0x1F];
    }
    else if (paddr < 0x04900000) {
        // Serial Interface
        if (paddr > 0x0480001B)
            return 0xFF;
        return SIreg[paddr & 0x1F];
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
        return PIFram[paddr & 0x3F];
    }
    else if (paddr < 0x1FD00000) {
        // Reserved
    }
    else if (paddr < 0x80000000) {
        // Cartridge Domain 1 Address 3
    }
    else {
        // Unknown
    }

    printf(" [ WRN ] Unknown Read from Physical Address 0x%08X. Debug mode enabled.\n", paddr);
    triggerDbgBrk = 1;
    return 0xFF;
}

void writePhys(u32 paddr, u8 val) {
    if (paddr < 0x00400000) {
        // RDRAM - built in
        RDRAM[paddr] = val;
        return;
    }
    else if (paddr < 0x00800000) {
        // RDRAM - expansion pak
        RDRAM[paddr] = val;
        return;
    }
    else if (paddr < 0x03F00000) {
        // Unused
    }
    else if (paddr < 0x04000000) {
        // RDRAM Registers
        if (paddr > 0x03F00027)
            return;
        else
            RDRAMreg[paddr & 0x3F] = val;
        return;
    }
    else if (paddr < 0x04001000) {
        // SP DMEM
        SPDmem[paddr & 0xFFF] = val;
        return;
    }
    else if (paddr < 0x04002000) {
        // SP IMEM
        SPImem[paddr & 0xFFF] = val;
        return;
    }
    else if (paddr < 0x04040000) {
        // Unused
    }
    else if (paddr < 0x04100000) {
        // SP Registers
        if (paddr > 0x04080008)
            return;
        if (paddr > 0x0404001F && paddr < 0x04080000)
            return;
        SPreg[paddr & 0x3FFFF] = val;
        return;
    }
    else if (paddr < 0x04200000) {
        // DP Command Registers
    }
    else if (paddr < 0x04300000) {
        // DP Span Registers
    }
    else if (paddr < 0x04400000) {
        // MIPS Interface
        if (paddr > 0x0430000F)
            return;

        switch (paddr) {
        case 0x04300008:
        case 0x04300009:
        case 0x0430000A:
        case 0x0430000B:
            return;

        case 0x0430000C:
        case 0x0430000D:
            return;
        case 0x0430000E:
            if (val & 1) mi_intr_reg &= ~((u64)1 << 4);
            if (val & 2) mi_intr_reg |= ((u64)1 << 4);
            if (val & 4) mi_intr_reg &= ~((u64)1 << 5);
            if (val & 8) mi_intr_reg |= ((u64)1 << 5);
            checkInterrupts();
            return;
        case 0x0430000F:
            if (val & 1) mi_intr_reg &= ~((u64)1 << 0);
            if (val & 2) mi_intr_reg |= ((u64)1 << 0);
            if (val & 4) mi_intr_reg &= ~((u64)1 << 1);
            if (val & 8) mi_intr_reg |= ((u64)1 << 1);
            if (val & 16) mi_intr_reg &= ~((u64)1 << 2);
            if (val & 32) mi_intr_reg |= ((u64)1 << 2);
            if (val & 64) mi_intr_reg &= ~((u64)1 << 3);
            if (val & 128) mi_intr_reg |= ((u64)1 << 3);
            checkInterrupts();
            return;
        }

        MIreg[paddr & 0xF] = val;
        return;
    }
    else if (paddr < 0x04500000) {
        // Video Interface
        if (paddr > 0x04400037)
            return;
        VIreg[paddr & 0x3F] = val;
        return;
    }
    else if (paddr < 0x04600000) {
        // Audio Interface
        if (paddr > 0x04500017)
            return;
        AIreg[paddr & 0x1F] = val;
        return;
    }
    else if (paddr < 0x04700000) {
        // Peripheral Interface
        if (paddr > 0x04600034)
            return;

        if (paddr >= 0x04600010 && paddr < 0x04600014)
            return;

        PIreg[paddr & 0x3F] = val;
        if (paddr == PI_BASE_REG + PI_WR_LEN_REG + 3) {
            u32 dramAddr = getu32((u32*)(PIreg + PI_DRAM_ADDR_REG));
            u32 cartAddr = getu32((u32*)(PIreg + PI_CART_ADDR_REG));
            u32 readLen = getu32((u32*)(PIreg + PI_WR_LEN_REG)) + 1;
            emuLog(" [ INF ] Initiating DMA from 0x%08X to 0x%08X (0x%08X bytes)\n", cartAddr, dramAddr, readLen);
            for (u32 i = 0; i < readLen; i++)
                writePhys(dramAddr + i, readPhys(cartAddr + i));
        }
        return;
    }
    else if (paddr < 0x04800000) {
        // RDRAM Interface
        if (paddr > 0x0470001F)
            return;
        RIreg[paddr & 0x1F] = val;
        return;
    }
    else if (paddr < 0x04900000) {
        // Serial Interface
        if (paddr > 0x0480001B)
            return;
        SIreg[paddr & 0x1F] = val;
        return;
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
        PIFram[paddr & 0x3F] = val;
        return;
    }
    else if (paddr < 0x1FD00000) {
        // Reserved
    }
    else if (paddr < 0x80000000) {
        // Cartridge Domain 1 Address 3
    }
    else {
        // Unknown
    }

    printf(" [ WRN ] Unknown Write to Physical Address 0x%08X at 0x%016llX. Debug Mode enabled.\n", paddr, pc);
    triggerDbgBrk = 1;
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

u32 getu32(u32* ptr) {
    u32 v = *ptr;
    return ((v & 0xFF000000) >> 24) | ((v & 0x00FF0000) >> 8) | ((v & 0x0000FF00) << 8) | ((v & 0x000000FF) << 24);
}