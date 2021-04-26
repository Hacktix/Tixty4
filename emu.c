#include "emu.h"
#include "mmu.h"
#include "cpu.h"
#include <stdio.h>
#include <stdlib.h>

void emuStart(FILE* romf) {
    mmuInit(romf);
    cpuInit();

    while (cpuExec() == 0) {
        // Execute Loop
    }
}