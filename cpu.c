#include "cpu.h"
#include "mmu.h"
#include <stdio.h>
#include <stdlib.h>

int cpuInit() {
	gpr = malloc(8 * 32);
	if (gpr == NULL)
		return -1;
	for (int i = 0; i < 31; i++)
		gpr[i] = 0;

	cop0Reg = malloc(8 * 32);
	if (cop0Reg == NULL)
		return -1;
	for (int i = 0; i < 31; i++)
		cop0Reg[i] = 0;

	delayQueue = 0;
	branchDecision = 0;

	cpuInitPIF();
	return 0;
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

	for (int i = 0; i < 0x1000; i++)
		writeu8(0xA4000000 + i, rom[i]);

	pc = 0xA4000040;
}

int cpuExec() {

	u32 instr = readu32(pc);
	u8 opcode = (instr >> 26) & 0x3F;
	pc += 4;

	if (instr != 0) {
		switch (opcode) {

		case 0x00: {
			u8 type = instr & 0x3F;
			switch (type) {
			case 0x08: instrJR(instr); break;
			case 0x25: instrOR(instr); break;
			default:
				printf("\n [ ERR ] Unimplemented Instruction 0x%08X at PC=0x%08X\n", instr, pc - 4);
				return -1;
			}
		}
		break;

		case 0x10: {
			u8 type = (instr >> 21) & 0x1F;
			switch (type) {
			case 0x04: instrMTC0(instr); break;
			default:
				printf("\n [ ERR ] Unimplemented Instruction 0x%08X at PC=0x%08X\n", instr, pc - 4);
				return -1;
			}
		}
		break;

		case 0x03: instrJAL(instr); break;
		case 0x05: instrBNE(instr); break;
		case 0x08: instrADDI(instr); break;
		case 0x09: instrADDIU(instr); break;
		case 0x0A: instrSLTI(instr); break;
		case 0x0D: instrORI(instr); break;
		case 0x0F: instrLUI(instr); break;
		case 0x14: instrBEQL(instr); break;
		case 0x23: instrLW(instr); break;
		case 0x2B: instrSW(instr); break;

		default:
			printf("\n [ ERR ] Unimplemented Instruction 0x%08X at PC=0x%08X\n", instr, pc - 4);
			return -1;
		}
	}
	else
		printf(" [ INF ] Executing: NOP\n");

	if (delayQueue > 0 && --delayQueue == 0) {
		if (branchDecision) {
			branchDecision = 0;
			pc = delaySlot;
			printf(" [ INF ] Jumping to 0x%08X (Branch Decision)\n", pc);
		}
	}

	return 0;
}

void instrMTC0(u32 instr) {
	char t = (instr >> 16) & 0x1F;
	char d = (instr >> 11) & 0x1F;
	printf(" [ INF ] Executing: MTC0 %02d, %02d [PC=0x%08X]\n", t, d, pc - 4);
	printf(" [ INF ]   Writing 0x%08X from GPR[%d] to CP0R[%d]\n", gpr[t], t, d);
	cop0Reg[d] = gpr[t];
}

void instrLUI(u32 instr) {
	char t = (instr >> 16) & 0x1F;
	u32 k = (instr & 0xFFFF) << 16;
	printf(" [ INF ] Executing: LUI %02d, %04X [PC=0x%08X]\n", t, (k >> 16) & 0xFFFF, pc - 4);
	printf(" [ INF ]   Writing 0x%08X to GPR[%d]\n", k, t);
	gpr[t] = k;
}

void instrADDIU(u32 instr) {
	char s = (instr >> 21) & 0x1F;
	char t = (instr >> 16) & 0x1F;
	i16 k = instr & 0xFFFF;
	i32 r = gpr[s] + k;
	printf(" [ INF ] Executing: ADDIU %02d, %02d, %04X [PC=0x%08X]\n", t, s, k, pc - 4);
	printf(" [ INF ]   Writing 0x%08X (=0x%08X+0x%08X) to GPR[%d]\n", r, gpr[s], k, t);
	gpr[t] = r;
}

void instrLW(u32 instr) {
	char b = (instr >> 21) & 0x1F;
	char t = (instr >> 16) & 0x1F;
	u16 f = instr & 0xFFFF;
	u32 addr = gpr[b] + f;
	u32 w = readu32(addr);
	printf(" [ INF ] Executing: LW %02d, %04X(%02d) [PC=0x%08X]\n", t, f, b, pc - 4);
	printf(" [ INF ]   Writing 0x%08X (from 0x%08X) to GPR[%d]\n", w, addr, t);
	gpr[t] = w;
}

void instrBNE(u32 instr) {
	char s = (instr >> 21) & 0x1F;
	char t = (instr >> 16) & 0x1F;
	i16 f = instr & 0xFFFF;
	delaySlot = pc + 4*f;
	branchDecision = (gpr[s] != gpr[t]);
	delayQueue = 2;
	printf(" [ INF ] Executing: BNE %02d, %02d, %d [PC=0x%08X]\n", s, t, f, pc - 4);
	printf(" [ INF ]   Writing 0x%08X to Delay Slot (Condition: %d)\n", delaySlot, branchDecision);
}

void instrSW(u32 instr) {
	char b = (instr >> 21) & 0x1F;
	char t = (instr >> 16) & 0x1F;
	u16 f = instr & 0xFFFF;
	u32 addr = gpr[b] + f;
	printf(" [ INF ] Executing: SW %02d, 0x%04X [PC=0x%08X]\n", t, f, pc - 4);
	printf(" [ INF ]   Writing 0x%08X from GPR[%d] to 0x%08X\n", gpr[t], t, addr);
	writeu32(addr, gpr[t]);
}

void instrORI(u32 instr) {
	char s = (instr >> 21) & 0x1F;
	char t = (instr >> 16) & 0x1F;
	u16 f = instr & 0xFFFF;
	u64 r = gpr[s] | (u64)f;
	printf(" [ INF ] Executing: ORI %02d, %02d, 0x%04X [PC=0x%08X]\n", t, s, f, pc - 4);
	printf(" [ INF ]   Writing 0x%08X (=0x%08X|0x%04X) to GPR[%d]\n", r, gpr[s], f, t);
	gpr[t] = r;
}

void instrADDI(u32 instr) {
	char s = (instr >> 21) & 0x1F;
	char t = (instr >> 16) & 0x1F;
	i16 k = instr & 0xFFFF;
	i32 r = gpr[s] + k;
	printf(" [ INF ] Executing: ADDI %02d, %02d, %04X [PC=0x%08X]\n", t, s, k, pc - 4);
	printf(" [ INF ]   Writing 0x%08X (=0x%08X+0x%08X) to GPR[%d]\n", r, gpr[s], k, t);
	gpr[t] = r;
}

void instrJAL(u32 instr) {
	u64 target = ((u64)(instr << 2) & 0xFFFFFFF);
	delaySlot = (pc & 0xFFFFFFFFF0000000) | target;
	delayQueue = 2;
	branchDecision = 1;
	printf(" [ INF ] Executing: JAL %07X [PC=0x%08X]\n", target, pc - 4);
	printf(" [ INF ]   Writing 0x%08X to Delay Slot, 0x%08X to GPR[31]\n", delaySlot, pc);
	gpr[GPR_RA] = pc;
}

void instrSLTI(u32 instr) {
	char s = (instr >> 21) & 0x1F;
	char t = (instr >> 16) & 0x1F;
	i64 k = (instr & 0xFFFF) | (instr & 0x8000 ? 0xFFFFFFFFFFFF0000 : 0);
	gpr[t] = ((i64)gpr[s]) < k;
	printf(" [ INF ] Executing: SLTI %02d, %02d, %04X [PC=0x%08X]\n", t, s, instr & 0xFFFF, pc - 4);
	printf(" [ INF ]   Writing %d (=0x%08X<0x%08X) to GPR[%d]\n", gpr[t], gpr[s], k, t);
}

void instrBEQL(u32 instr) {
	char s = (instr >> 21) & 0x1F;
	char t = (instr >> 16) & 0x1F;
	i16 f = instr & 0xFFFF;
	delaySlot = pc + 4 * f;
	branchDecision = (gpr[s] != gpr[t]);
	delayQueue = 2;
	if (!branchDecision)
		pc += 4;
	printf(" [ INF ] Executing: BEQL %02d, %02d, %d [PC=0x%08X]\n", s, t, f, pc - 4);
	printf(" [ INF ]   Writing 0x%08X to Delay Slot (Condition: %d)\n", delaySlot, branchDecision);
}

void instrJR(u32 instr) {
	char s = (instr >> 21) & 0x1F;
	delaySlot = gpr[s];
	branchDecision = 1;
	delayQueue = 2;
	printf(" [ INF ] Executing: JR %d [PC=0x%08X]\n", s, pc - 4);
	printf(" [ INF ]   Writing 0x%08X to Delay Slot\n", delaySlot);
}



void instrOR(u32 instr) {
	char s = (instr >> 21) & 0x1F;
	char t = (instr >> 16) & 0x1F;
	char d = (instr >> 11) & 0x1F;
	u64 r = gpr[s] | gpr[t];
	printf(" [ INF ] Executing: OR %02d, %02d, %02d [PC=0x%08X]\n", d, s, t, pc - 4);
	printf(" [ INF ]   Writing 0x%08X (=0x%08X|0x%08X) to GPR[%d]\n", r, gpr[s], gpr[t], d);
	gpr[d] = r;
}