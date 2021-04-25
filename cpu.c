#include "cpu.h"
#include <stdio.h>
#include <stdlib.h>

void cpuInit() {
	gpr = malloc(8 * 32);
	for (int i = 0; i < 31; i++)
		gpr[i] = 0;

	cop0Reg = malloc(8 * 32);
	for (int i = 0; i < 31; i++)
		cop0Reg[i] = 0;

	cpuInitPIF();
}

void cpuInitPIF() {
	gpr[GPR_T3] = 0xFFFFFFFFA4000040;
	gpr[GPR_S4] = 0x0000000000000001;
	gpr[GPR_S6] = 0x000000000000003F;
	gpr[GPR_SP] = 0xFFFFFFFFA4001FF0;

	cop0Reg[CP0R_Random] = 0x0000001F;
	cop0Reg[CP0R_Status] = 0x70400004;
	cop0Reg[CP0R_PRId] = 0x00000B00;
	cop0Reg[CP0R_Config] = 0x0006E463;

	pc = 0xA4000040;
}