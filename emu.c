#include "emu.h"
#include "mmu.h"
#include "cpu.h"
#include <stdio.h>
#include <stdlib.h>

#define DBG_BRK 0xA40004A0

void emuStart(FILE* romf) {
    hitDbgBrk = 0;
    mmuInit(romf);
    cpuInit();

    while (cpuExec() == 0) {
        // Execute Loop
        if (pc == DBG_BRK) {
            hitDbgBrk = 1;
            printf(" [ INF ] Hit Debug Breakpoint at 0x%08X\n", pc);
        }
        if (hitDbgBrk) {
            char pressed = getchar();
            if (pressed == 0x63)
                hitDbgBrk = 0;
        }
    }
}