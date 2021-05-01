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
const char* gprName[] = { "R0", "AT", "V0", "V1", "A0", "A1", "A2", "A3", "T0", "T1", "T2", "T3", "T4", "T5", "T6", "T7", "S0", "S1", "S2", "S3", "S4", "S5", "S6", "S7", "T8", "T9", "K0", "K1", "GP", "SP", "FP", "RA" };

int emuStart(FILE* romf) {
    hitDbgBrk = 0;
    triggerDbgBrk = 0;

    initUI();
    mmuInit(romf);
    cpuInit();

    clock_t time = clock();
    SDL_Event uiEvent;

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
            else {
                printf("GPR\n");
                for (u8 i = 0; i < 32; i++)
                    printf("%s %08X %08X\n", gprName[i], gpr[i] >> 32, gpr[i]);
                printf("HI %08X %08X\n", hiReg >> 32, hiReg);
                printf("LO %08X %08X\n", loReg >> 32, loReg);
            }
        }
        if (++drawCounter == 1000000) {
            // Render Framebuffer
            drawFramebuffer();
            drawCounter = 0;

            // Handle SDL Events
            SDL_PollEvent(&uiEvent);
            switch (uiEvent.type) {
            case SDL_QUIT:
                return 0;
            case SDL_KEYDOWN:
                if (uiEvent.key.keysym.scancode == 41)
                    hitDbgBrk = 1;
                break;
            }
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