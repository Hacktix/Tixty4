#include "emu.h"
#include "mmu.h"
#include "cpu.h"
#include "ui.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>

#define DBG_BRK 0

u32 drawCounter = 0;

void emuStart(FILE* romf) {
    hitDbgBrk = 0;
    triggerDbgBrk = 0;

    initUI();
    mmuInit(romf);
    cpuInit();

    clock_t time = clock();

    while (cpuExec() == 0) {
        // Execute Loop
        if ((pc & 0xFFFFFFFF) == DBG_BRK || triggerDbgBrk) {
            time = clock() - time;
            hitDbgBrk = 1;
            printf(" [ INF ] Hit Debug Breakpoint at 0x%08X after %dms\n", pc, time);
        }
        if (hitDbgBrk) {
            char pressed = getchar();
            if (pressed == 0x63) {
                time = clock();
                hitDbgBrk = 0;
            }
        }
        if (++drawCounter == 1000000) {
            drawFramebuffer();
            drawCounter = 0;
            SDL_PollEvent(NULL);
        }
    }
}

void emuLog(const char* fmt, ...) {
    if (hitDbgBrk) {
        va_list pargs;
        va_start(pargs, fmt);
        vprintf(fmt, pargs);
        va_end(pargs);
    }
}