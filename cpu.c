#include "cpu.h"
#include "mmu.h"
#include "signext.h"
#include "emu.h"
#include <stdio.h>
#include <stdlib.h>
#include <fenv.h>

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

	fgr = malloc(8 * 32);
	if (fgr == NULL)
		return -1;
	for (int i = 0; i < 64; i++)
		fgr[i] = 0;

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

	gpr[0] = 0;
	u32 instr = readu32(pc);
	u8 opcode = (instr >> 26) & 0x3F;
	pc += 4;
	emuLog("\n [ INF ] Read Instruction: 0x%08X\n", instr);

	if (instr != 0) {
		switch (opcode) {

		case 0x00: {
			u8 type = instr & 0x3F;
			switch (type) {
			case 0x00: instrSLL(instr); break;
			case 0x02: instrSRL(instr); break;
			case 0x03: instrSRA(instr); break;
			case 0x04: instrSLLV(instr); break;
			case 0x06: instrSRLV(instr); break;
			case 0x08: instrJR(instr); break;
			case 0x09: instrJALR(instr); break;
			case 0x10: instrMFHI(instr); break;
			case 0x11: instrMTHI(instr); break;
			case 0x12: instrMFLO(instr); break;
			case 0x13: instrMTLO(instr); break;
			case 0x18: instrMULT(instr); break;
			case 0x19: instrMULTU(instr); break;
			case 0x1A: instrDIV(instr); break;
			case 0x1B: instrDIVU(instr); break;
			case 0x1E: instrDDIV(instr); break;
			case 0x1F: instrDDIVU(instr); break;
			case 0x20: instrADD(instr); break;
			case 0x21: instrADDU(instr); break;
			case 0x22: instrSUB(instr); break;
			case 0x23: instrSUBU(instr); break;
			case 0x24: instrAND(instr); break;
			case 0x25: instrOR(instr); break;
			case 0x26: instrXOR(instr); break;
			case 0x2A: instrSLT(instr); break;
			case 0x2B: instrSLTU(instr); break;
			case 0x2C: instrDADD(instr); break;
			case 0x2D: instrDADDU(instr); break;
			default:
				hitDbgBrk = 1;
				emuLog("\n [ ERR ] Unimplemented Instruction 0x%016llX (Opcode %02X, Type %02X) at PC=0x%016llX\n", instr, opcode, type, pc - 4);
				return -1;
			}
		}
		break;

		case 0x10: {
			if (instr == 0x42000018) {
				instrERET(instr);
				break;
			}
			u8 type = (instr >> 21) & 0x1F;
			switch (type) {
			case 0x00: instrDMFC0(instr); break;
			case 0x04: instrMTC0(instr); break;
			case 0x10: instrTLBWI(instr); break;
			default:
				hitDbgBrk = 1;
				emuLog("\n [ ERR ] Unimplemented Instruction 0x%016llX (Opcode %02X, Type %02X) at PC=0x%016llX\n", instr, opcode, type, pc - 4);
				return -1;
			}
		}
		break;

		case 0x11: {
			if ((instr & 0b1111'0011'1110'0000'0000'0111'1111'1111) == 0b0100'0000'0100'0000'0000'0000'0000'0000) instrCFC(instr);
			else if ((instr & 0b1111'0011'1110'0000'0000'0111'1111'1111) == 0b0100'0000'1100'0000'0000'0000'0000'0000) instrCTC(instr);
			else if ((instr & 0b1111'1111'1110'0000'0000'0111'1111'1111) == 0b0100'0100'1000'0000'0000'0000'0000'0000) instrMTC1(instr);
			else if ((instr & 0b1111'1111'1110'0000'0000'0111'1111'1111) == 0b0100'0100'0000'0000'0000'0000'0000'0000) instrMFC1(instr);
			else if ((instr & 0b1111'1111'1111'1111'0000'0000'0011'1111) == 0b0100'0110'0000'0000'0000'0000'0010'0001) instrCVT_D_S(instr);
			else if ((instr & 0b1111'1111'1111'1111'0000'0000'0011'1111) == 0b0100'0110'1000'0000'0000'0000'0010'0001) instrCVT_D_W(instr);
			else if ((instr & 0b1111'1111'1111'1111'0000'0000'0011'1111) == 0b0100'0110'1010'0000'0000'0000'0010'0001) instrCVT_D_L(instr);
			else if ((instr & 0b1111'1111'1111'1111'0000'0000'0011'1111) == 0b0100'0110'1000'0000'0000'0000'0010'0000) instrCVT_S_W(instr);
			else if ((instr & 0b1111'1111'1111'1111'0000'0000'0011'1111) == 0b0100'0110'1010'0000'0000'0000'0010'0000) instrCVT_S_L(instr);
			else if ((instr & 0b1111'1111'1111'1111'0000'0000'0011'1111) == 0b0100'0110'0010'0000'0000'0000'0010'0000) instrCVT_S_D(instr);
			else if ((instr & 0b1111'1111'1111'1111'0000'0000'0011'1111) == 0b0100'0110'0000'0000'0000'0000'0010'0100) instrCVT_W_S(instr);
			else if ((instr & 0b1111'1111'1111'1111'0000'0000'0011'1111) == 0b0100'0110'0010'0000'0000'0000'0010'0100) instrCVT_W_D(instr);
			else if ((instr & 0b1111'1111'1111'1111'0000'0000'0011'1111) == 0b0100'0110'0000'0000'0000'0000'0010'0101) instrCVT_L_S(instr);
			else if ((instr & 0b1111'1111'1111'1111'0000'0000'0011'1111) == 0b0100'0110'0010'0000'0000'0000'0010'0101) instrCVT_L_D(instr);
			else if ((instr & 0b1111'1111'1110'0000'0000'0000'0011'1111) == 0b0100'0110'0000'0000'0000'0000'0000'0011) instrDIV_S(instr);
			else if ((instr & 0b1111'1111'1110'0000'0000'0000'0011'1111) == 0b0100'0110'0010'0000'0000'0000'0000'0011) instrDIV_D(instr);
			else if ((instr & 0b1111'1111'1110'0000'0000'0000'0011'1111) == 0b0100'0110'0000'0000'0000'0000'0000'0000) instrADD_S(instr);
			else if ((instr & 0b1111'1111'1110'0000'0000'0000'0011'1111) == 0b0100'0110'0010'0000'0000'0000'0000'0000) instrADD_D(instr);
			else if ((instr & 0b1111'1111'1110'0000'0000'0000'0011'1111) == 0b0100'0110'0010'0000'0000'0000'0000'1001) instrTRUNC_L_D(instr);
			else if ((instr & 0b1111'1111'1110'0000'0000'0000'0011'1111) == 0b0100'0110'0000'0000'0000'0000'0000'1001) instrTRUNC_L_S(instr);
			else if ((instr & 0b1111'1111'1110'0000'0000'0000'0011'1111) == 0b0100'0110'0010'0000'0000'0000'0000'1101) instrTRUNC_W_D(instr);
			else if ((instr & 0b1111'1111'1110'0000'0000'0000'0011'1111) == 0b0100'0110'0000'0000'0000'0000'0000'1101) instrTRUNC_W_S(instr);
			else if ((instr & 0b1111'1111'1110'0000'0000'0111'1111'1111) == 0b0100'0110'0000'0000'0000'0000'0011'1110) instrC_LE_S(instr);
			else if ((instr & 0b1111'1111'1110'0000'0000'0111'1111'1111) == 0b0100'0110'0010'0000'0000'0000'0011'1110) instrC_LE_D(instr);
			else if ((instr & 0xFFFF0000) == 0b0100'0101'0000'0001'0000'0000'0000'0000) instrBC1T(instr);
			else if ((instr & 0xFFFF0000) == 0b0100'0101'0000'0011'0000'0000'0000'0000) instrBC1TL(instr);
			else if ((instr & 0xFFFF0000) == 0b0100'0101'0000'0000'0000'0000'0000'0000) instrBC1F(instr);
			else {
				hitDbgBrk = 1;
				emuLog("\n [ ERR ] Unimplemented Instruction 0x%016llX at PC=0x%016llX\n", instr, pc - 4);
				return -1;
			}
		}
		break;

		case 0x01: instrBGEZL(instr); break;
		case 0x02: instrJ(instr); break;
		case 0x03: instrJAL(instr); break;
		case 0x04: instrBEQ(instr); break;
		case 0x05: instrBNE(instr); break;
		case 0x06: instrBLEZ(instr); break;
		case 0x07: instrBGTZ(instr); break;
		case 0x08: instrADDI(instr); break;
		case 0x09: instrADDIU(instr); break;
		case 0x0A: instrSLTI(instr); break;
		case 0x0B: instrSLTIU(instr); break;
		case 0x0C: instrANDI(instr); break;
		case 0x0D: instrORI(instr); break;
		case 0x0E: instrXORI(instr); break;
		case 0x0F: instrLUI(instr); break;
		case 0x14: instrBEQL(instr); break;
		case 0x15: instrBNEL(instr); break;
		case 0x16: instrBLEZL(instr); break;
		case 0x18: instrDADDI(instr); break;
		case 0x19: instrDADDIU(instr); break;
		case 0x20: instrLB(instr); break;
		case 0x21: instrLH(instr); break;
		case 0x23: instrLW(instr); break;
		case 0x24: instrLBU(instr); break;
		case 0x25: instrLHU(instr); break;
		case 0x27: instrLWU(instr); break;
		case 0x28: instrSB(instr); break;
		case 0x29: instrSH(instr); break;
		case 0x2B: instrSW(instr); break;
		case 0x2F: instrCACHE(instr); break;
		case 0x31: instrLWC1(instr); break;
		case 0x35: instrLDC1(instr); break;
		case 0x37: instrLD(instr); break;
		case 0x39: instrSWC1(instr); break;
		case 0x3D: instrSDC1(instr); break;
		case 0x3F: instrSD(instr); break;

		default:
			hitDbgBrk = 1;
			emuLog("\n [ ERR ] Unimplemented Instruction 0x%016llX (Opcode %02X) at PC=0x%016llX\n", instr, opcode, pc - 4);
			return -1;
		}
	}
	else
		emuLog(" [ INF ] Executing: NOP [PC=0x%016llX]\n", pc-4);

	if (delayQueue > 0 && --delayQueue == 0) {
		if (branchDecision) {
			branchDecision = 0;
			pc = delaySlot;
			emuLog(" [ INF ] Jumping to 0x%016llX (Branch Decision)\n", pc);
		}
	}

	return 0;
}

float getFPR_S(u64 index) {
	u32 v = getFPR_W(index);
	return *((float*)&v);
}

double getFPR_D(u64 index) {
	u64 v = getFPR_L(index);
	return *((double*)&v);
}

u32 getFPR_W(u64 index) {
	if ((cop0Reg[CP0R_Status] >> 26) & 1)
		return fgr[2 * index];
	else
		return fgr[4 * index];
}

u64 getFPR_L(u64 index) {
	if ((cop0Reg[CP0R_Status] >> 26) & 1)
		return *((u64*)(fgr + 2 * index));
	else
		return fgr[4 * index] | (((u64)fgr[4 * index + 2]) << 32);
}

void setFPR_S(u64 index, float value) {
	u32 fval = *((u32*)&value);
	setFPR_W(index, fval);
}

void setFPR_D(u64 index, double value) {
	u64 dval = *((u64*)&value);
	setFPR_L(index, dval);
}

void setFPR_W(u64 index, u32 value) {
	if ((cop0Reg[CP0R_Status] >> 26) & 1) {
		fgr[2 * index] = value;
		fgr[2 * index + 1] = 0;
	}
	else {
		fgr[4 * index] = value;
		fgr[4 * index + 2] = 0;
	}
}

void setFPR_L(u64 index, u64 value) {
	if ((cop0Reg[CP0R_Status] >> 26) & 1) {
		u64* vptr = fgr + 2 * index;
		*vptr = value;
	}
	else {
		fgr[4 * index] = (u32)value;
		fgr[4 * index + 2] = (u32)(value >> 32);
	}
}

void setFCR31(u32 val) {
	fcr31 = val;

	// Set Rounding Mode
	switch (val & 0b11) {
	case 0b00:
		fesetround(FE_TONEAREST);
		emuLog(" [ INF ] Set FPU Rounding Mode to RN (FE_TONEAREST)\n", 0);
		break;
	case 0x01:
		fesetround(FE_TOWARDZERO);
		emuLog(" [ INF ] Set FPU Rounding Mode to RZ (FE_TOWARDZERO)\n", 0);
		break;
	case 0b10:
		fesetround(FE_UPWARD);
		emuLog(" [ INF ] Set FPU Rounding Mode to RP (FE_UPWARD)\n", 0);
		break;
	case 0b11:
		fesetround(FE_DOWNWARD);
		emuLog(" [ INF ] Set FPU Rounding Mode to RM (FE_DOWNWARD)\n", 0);
		break;
	}
}

void instrMTC0(u32 instr) {
	char t = (instr >> 16) & 0x1F;
	char d = (instr >> 11) & 0x1F;
	emuLog(" [ INF ] Executing: MTC0 %02d, %02d [PC=0x%016llX]\n", t, d, pc - 4);
	emuLog(" [ INF ]   Writing 0x%016llX from GPR[%d] to CP0R[%d]\n", gpr[t], t, d);
	cop0Reg[d] = gpr[t];
}

void instrLUI(u32 instr) {
	char t = (instr >> 16) & 0x1F;
	u64 k = s32ext64((instr & 0xFFFF) << 16);
	emuLog(" [ INF ] Executing: LUI %02d, %04X [PC=0x%016llX]\n", t, (k >> 16) & 0xFFFF, pc - 4);
	emuLog(" [ INF ]   Writing 0x%016llX to GPR[%d]\n", k, t);
	gpr[t] = k;
}

void instrADDIU(u32 instr) {
	char s = (instr >> 21) & 0x1F;
	char t = (instr >> 16) & 0x1F;
	i32 k = (i32)s16ext32(instr & 0xFFFF);
	u64 r = s32ext64((u32)(gpr[s] + k));
	emuLog(" [ INF ] Executing: ADDIU %02d, %02d, %04X [PC=0x%016llX]\n", t, s, k, pc - 4);
	emuLog(" [ INF ]   Writing 0x%016llX (=0x%08X+0x%08X) to GPR[%d]\n", r, gpr[s], k, t);
	gpr[t] = r;
}

void instrLW(u32 instr) {
	char b = (instr >> 21) & 0x1F;
	char t = (instr >> 16) & 0x1F;
	i32 f = (i32)s16ext32(instr & 0xFFFF);
	u32 addr = gpr[b] + f;
	u32 w = readu32(addr);
	u64 r = s32ext64(w);
	emuLog(" [ INF ] Executing: LW %02d, %04X(%02d) [PC=0x%016llX]\n", t, f, b, pc - 4);
	emuLog(" [ INF ]   Writing 0x%016llX (0x%08X read from 0x%016llX) to GPR[%d]\n", r, w, addr, t);
	gpr[t] = r;
}

void instrBNE(u32 instr) {
	char s = (instr >> 21) & 0x1F;
	char t = (instr >> 16) & 0x1F;
	i64 f = (i64)s16ext64(instr & 0xFFFF);
	delaySlot = pc + 4*f;
	branchDecision = (gpr[s] != gpr[t]);
	delayQueue = 2;
	emuLog(" [ INF ] Executing: BNE %02d, %02d, %d [PC=0x%016llX]\n", s, t, f, pc - 4);
	emuLog(" [ INF ]   Writing 0x%016llX to Delay Slot (Condition: %d | 0x%016llX != 0x%016llX)\n", delaySlot, branchDecision, gpr[s], gpr[t]);
}

void instrSW(u32 instr) {
	char b = (instr >> 21) & 0x1F;
	char t = (instr >> 16) & 0x1F;
	i16 f = instr & 0xFFFF;
	u32 addr = gpr[b] + f;
	emuLog(" [ INF ] Executing: SW %02d, 0x%04X [PC=0x%016llX]\n", t, f, pc - 4);
	emuLog(" [ INF ]   Writing 0x%016llX from GPR[%d] to 0x%016llX\n", gpr[t], t, addr);
	writeu32(addr, gpr[t]);
}

void instrSH(u32 instr) {
	char b = (instr >> 21) & 0x1F;
	char t = (instr >> 16) & 0x1F;
	i16 f = instr & 0xFFFF;
	u32 addr = gpr[b] + f;
	emuLog(" [ INF ] Executing: SH %02d, 0x%04X [PC=0x%016llX]\n", t, f, pc - 4);
	emuLog(" [ INF ]   Writing 0x%04X from GPR[%d] to 0x%016llX\n", gpr[t], t, addr);
	writeu16(addr, gpr[t]);
}

void instrORI(u32 instr) {
	char s = (instr >> 21) & 0x1F;
	char t = (instr >> 16) & 0x1F;
	u16 f = instr & 0xFFFF;
	u64 r = gpr[s] | (u64)f;
	emuLog(" [ INF ] Executing: ORI %02d, %02d, 0x%04X [PC=0x%016llX]\n", t, s, f, pc - 4);
	emuLog(" [ INF ]   Writing 0x%016llX (=0x%016llX|0x%04X) to GPR[%d]\n", r, gpr[s], f, t);
	gpr[t] = r;
}

void instrADDI(u32 instr) {
	char s = (instr >> 21) & 0x1F;
	char t = (instr >> 16) & 0x1F;
	i32 k = (i32)s16ext32(instr & 0xFFFF);
	u64 r = s32ext64((u32)(gpr[s] + k));
	emuLog(" [ INF ] Executing: ADDI %02d, %02d, %04X [PC=0x%016llX]\n", t, s, k, pc - 4);
	emuLog(" [ INF ]   Writing 0x%016llX (=0x%016llX+0x%016llX) to GPR[%d]\n", r, gpr[s], k, t);
	gpr[t] = r;
}

void instrJAL(u32 instr) {
	u64 target = ((u64)(((u64)instr) << 2) & 0xFFFFFFF);
	delaySlot = (pc & 0xFFFFFFFFF0000000) | target;
	delayQueue = 2;
	branchDecision = 1;
	emuLog(" [ INF ] Executing: JAL %07X [PC=0x%016llX]\n", target, pc - 4);
	emuLog(" [ INF ]   Writing 0x%016llX to Delay Slot, 0x%016llX to GPR[31]\n", delaySlot, pc + 4);
	gpr[GPR_RA] = pc + 4;
}

void instrSLTI(u32 instr) {
	char s = (instr >> 21) & 0x1F;
	char t = (instr >> 16) & 0x1F;
	i64 k = s16ext64(instr & 0xFFFF);
	gpr[t] = ((i64)gpr[s]) < k;
	emuLog(" [ INF ] Executing: SLTI %02d, %02d, %04X [PC=0x%016llX]\n", t, s, instr & 0xFFFF, pc - 4);
	emuLog(" [ INF ]   Writing %d (=0x%016llX<0x%016llX) to GPR[%d]\n", gpr[t], gpr[s], k, t);
}

void instrBEQL(u32 instr) {
	char s = (instr >> 21) & 0x1F;
	char t = (instr >> 16) & 0x1F;
	i64 f = (i64)s16ext64(instr & 0xFFFF);
	delaySlot = pc + 4 * f;
	branchDecision = (gpr[s] == gpr[t]);
	delayQueue = 2;
	if (!branchDecision)
		pc += 4;
	emuLog(" [ INF ] Executing: BEQL %02d, %02d, %d [PC=0x%016llX]\n", s, t, f, pc - 4);
	emuLog(" [ INF ]   Writing 0x%016llX to Delay Slot (Condition: %d | 0x%016llX == 0x%016llX)\n", delaySlot, branchDecision, gpr[s], gpr[t]);
}

void instrBNEL(u32 instr) {
	char s = (instr >> 21) & 0x1F;
	char t = (instr >> 16) & 0x1F;
	i64 f = (i64)s16ext64(instr & 0xFFFF);
	delaySlot = pc + 4 * f;
	branchDecision = (gpr[s] != gpr[t]);
	delayQueue = 2;
	if (!branchDecision)
		pc += 4;
	emuLog(" [ INF ] Executing: BNEL %02d, %02d, %d [PC=0x%016llX]\n", s, t, f, pc - 4);
	emuLog(" [ INF ]   Writing 0x%016llX to Delay Slot (Condition: %d | 0x%016llX != 0x%016llX)\n", delaySlot, branchDecision, gpr[s], gpr[t]);
}

void instrBLEZL(u32 instr) {
	char s = (instr >> 21) & 0x1F;
	i64 f = (i64)s16ext64(instr & 0xFFFF);
	delaySlot = pc + 4 * f;
	branchDecision = ((i64)gpr[s]) <= 0;
	delayQueue = 2;
	if (!branchDecision)
		pc += 4;
	emuLog(" [ INF ] Executing: BLEZL %02d, %d [PC=0x%016llX]\n", s, f, pc - 4);
	emuLog(" [ INF ]   Writing 0x%016llX to Delay Slot (Condition: %d | 0x%016llX <= 0)\n", delaySlot, branchDecision, gpr[s]);
}

void instrJR(u32 instr) {
	char s = (instr >> 21) & 0x1F;
	delaySlot = gpr[s];
	branchDecision = 1;
	delayQueue = 2;
	emuLog(" [ INF ] Executing: JR %d [PC=0x%016llX]\n", s, pc - 4);
	emuLog(" [ INF ]   Writing 0x%016llX to Delay Slot\n", delaySlot);
}

void instrANDI(u32 instr) {
	char s = (instr >> 21) & 0x1F;
	char t = (instr >> 16) & 0x1F;
	u16 f = instr & 0xFFFF;
	u64 r = gpr[s] & (u64)f;
	emuLog(" [ INF ] Executing: ANDI %02d, %02d, 0x%04X [PC=0x%016llX]\n", t, s, f, pc - 4);
	emuLog(" [ INF ]   Writing 0x%016llX (=0x%016llX&0x%04X) to GPR[%d]\n", r, gpr[s], f, t);
	gpr[t] = r;
}

void instrXORI(u32 instr) {
	char s = (instr >> 21) & 0x1F;
	char t = (instr >> 16) & 0x1F;
	u16 f = instr & 0xFFFF;
	u64 r = gpr[s] ^ (u64)f;
	emuLog(" [ INF ] Executing: XORI %02d, %02d, 0x%04X [PC=0x%016llX]\n", t, s, f, pc - 4);
	emuLog(" [ INF ]   Writing 0x%016llX (=0x%016llX^0x%04X) to GPR[%d]\n", r, gpr[s], f, t);
	gpr[t] = r;
}

void instrBEQ(u32 instr) {
	char s = (instr >> 21) & 0x1F;
	char t = (instr >> 16) & 0x1F;
	i64 f = (i64)s16ext64(instr & 0xFFFF);
	delaySlot = pc + 4 * f;
	branchDecision = (gpr[s] == gpr[t]);
	delayQueue = 2;
	emuLog(" [ INF ] Executing: BEQ %02d, %02d, %d [PC=0x%016llX]\n", s, t, f, pc - 4);
	emuLog(" [ INF ]   Writing 0x%016llX to Delay Slot (Condition: %d | 0x%016llX == 0x%016llX)\n", delaySlot, branchDecision, gpr[s], gpr[t]);
}

void instrCACHE(u32 instr) {
	emuLog(" [ INF ] Executing: CACHE [PC=0x%016llX]\n", pc - 4);
	emuLog(" [ INF ]   NOTE: This instruction isn't implemented yet.\n", 0);
}

void instrSB(u32 instr) {
	char b = (instr >> 21) & 0x1F;
	char t = (instr >> 16) & 0x1F;
	i16 f = instr & 0xFFFF;
	u32 addr = gpr[b] + f;
	u8 v = (u8)gpr[t];
	emuLog(" [ INF ] Executing: SB %02d, 0x%04X [PC=0x%016llX]\n", t, f, pc - 4);
	emuLog(" [ INF ]   Writing 0x%02X from GPR[%d] to 0x%016llX\n", v, t, addr);
	writeu8(addr, v);
}

void instrLBU(u32 instr) {
	char b = (instr >> 21) & 0x1F;
	char t = (instr >> 16) & 0x1F;
	i16 f = instr & 0xFFFF;
	u32 addr = gpr[b] + f;
	u8 v = readu8(addr);
	emuLog(" [ INF ] Executing: LBU %02d, 0x%04X [PC=0x%016llX]\n", t, f, pc - 4);
	emuLog(" [ INF ]   Writing 0x%02X from 0x%016llX to GPR[%d]\n", v, addr, t);
	gpr[t] = v;
}

void instrBGEZL(u32 instr) {
	char s = (instr >> 21) & 0x1F;
	i64 f = (i64)s16ext64(instr & 0xFFFF);
	delaySlot = pc + 4 * f;
	branchDecision = ((i64)gpr[s]) >= 0;
	delayQueue = 2;
	if (!branchDecision)
		pc += 4;
	emuLog(" [ INF ] Executing: BGEZL %02d, %d [PC=0x%016llX]\n", s, f, pc - 4);
	emuLog(" [ INF ]   Writing 0x%016llX to Delay Slot (Condition: %d | 0x%016llX >= 0)\n", delaySlot, branchDecision, gpr[s]);
}

void instrJ(u32 instr) {
	u64 target = ((u64)(((u64)instr) << 2) & 0xFFFFFFF);
	delaySlot = (pc & 0xFFFFFFFFF0000000) | target;
	delayQueue = 2;
	branchDecision = 1;
	emuLog(" [ INF ] Executing: J %07X [PC=0x%016llX]\n", target, pc - 4);
	emuLog(" [ INF ]   Writing 0x%016llX to Delay Slot\n", delaySlot, pc);
}

void instrLB(u32 instr) {
	char b = (instr >> 21) & 0x1F;
	char t = (instr >> 16) & 0x1F;
	i16 f = instr & 0xFFFF;
	u32 addr = gpr[b] + f;
	u64 v = s8ext64(readu8(addr));
	emuLog(" [ INF ] Executing: LB %02d, 0x%04X [PC=0x%016llX]\n", t, f, pc - 4);
	emuLog(" [ INF ]   Writing 0x%016llX from 0x%016llX to GPR[%d]\n", v, addr, t);
	gpr[t] = v;
}

void instrLH(u32 instr) {
	char b = (instr >> 21) & 0x1F;
	char t = (instr >> 16) & 0x1F;
	i16 f = instr & 0xFFFF;
	u32 addr = gpr[b] + f;
	u64 v = s16ext64(readu16(addr));
	emuLog(" [ INF ] Executing: LH %02d, 0x%04X [PC=0x%016llX]\n", t, f, pc - 4);
	emuLog(" [ INF ]   Writing 0x%016llX from 0x%016llX to GPR[%d]\n", v, addr, t);
	gpr[t] = v;
}

void instrBGTZ(u32 instr) {
	char s = (instr >> 21) & 0x1F;
	i64 f = (i64)s16ext64(instr & 0xFFFF);
	delaySlot = pc + 4 * f;
	branchDecision = ((i64)gpr[s]) > 0;
	delayQueue = 2;
	emuLog(" [ INF ] Executing: BGTZ %02d, %d [PC=0x%016llX]\n", s, f, pc - 4);
	emuLog(" [ INF ]   Writing 0x%016llX to Delay Slot (Condition: %d | 0x%016llX > 0)\n", delaySlot, branchDecision, gpr[s]);
}

void instrLWU(u32 instr) {
	char b = (instr >> 21) & 0x1F;
	char t = (instr >> 16) & 0x1F;
	i32 f = (i32)s16ext32(instr & 0xFFFF);
	u32 addr = gpr[b] + f;
	u32 w = readu32(addr);
	u64 r = (u64)w;
	emuLog(" [ INF ] Executing: LWU %02d, %04X(%02d) [PC=0x%016llX]\n", t, f, b, pc - 4);
	emuLog(" [ INF ]   Writing 0x%016llX (0x%08X read from 0x%016llX) to GPR[%d]\n", r, w, addr, t);
	gpr[t] = r;
}

void instrDADDI(u32 instr) {
	char s = (instr >> 21) & 0x1F;
	char t = (instr >> 16) & 0x1F;
	i32 k = (i32)s16ext32(instr & 0xFFFF);
	u64 r = gpr[s] + k;
	emuLog(" [ INF ] Executing: DADDI %02d, %02d, %04X [PC=0x%016llX]\n", t, s, k, pc - 4);
	emuLog(" [ INF ]   Writing 0x%016llX (=0x%016llX+0x%016llX) to GPR[%d]\n", r, gpr[s], k, t);
	gpr[t] = r;
}

void instrLHU(u32 instr) {
	char b = (instr >> 21) & 0x1F;
	char t = (instr >> 16) & 0x1F;
	i16 f = instr & 0xFFFF;
	u32 addr = gpr[b] + f;
	u16 v = readu16(addr);
	emuLog(" [ INF ] Executing: LHU %02d, 0x%04X [PC=0x%016llX]\n", t, f, pc - 4);
	emuLog(" [ INF ]   Writing 0x%04X from 0x%016llX to GPR[%d]\n", v, addr, t);
	gpr[t] = v;
}

void instrLD(u32 instr) {
	char b = (instr >> 21) & 0x1F;
	char t = (instr >> 16) & 0x1F;
	i32 f = (i32)s16ext32(instr & 0xFFFF);
	u32 addr = gpr[b] + f;
	u64 w = readu64(addr);
	emuLog(" [ INF ] Executing: LD %02d, %04X(%02d) [PC=0x%016llX]\n", t, f, b, pc - 4);
	emuLog(" [ INF ]   Writing 0x%016llX (read from 0x%016llX) to GPR[%d]\n", w, addr, t);
	gpr[t] = w;
}

void instrDADDIU(u32 instr) {
	char s = (instr >> 21) & 0x1F;
	char t = (instr >> 16) & 0x1F;
	i32 k = (i32)s16ext32(instr & 0xFFFF);
	u64 r = gpr[s] + k;
	emuLog(" [ INF ] Executing: DADDIU %02d, %02d, %04X [PC=0x%016llX]\n", t, s, k, pc - 4);
	emuLog(" [ INF ]   Writing 0x%016llX (=0x%016llX+0x%016llX) to GPR[%d]\n", r, gpr[s], k, t);
	gpr[t] = r;
}

void instrSLTIU(u32 instr) {
	char s = (instr >> 21) & 0x1F;
	char t = (instr >> 16) & 0x1F;
	i64 k = s16ext64(instr & 0xFFFF);
	gpr[t] = gpr[s] < k;
	emuLog(" [ INF ] Executing: SLTIU %02d, %02d, %04X [PC=0x%016llX]\n", t, s, instr & 0xFFFF, pc - 4);
	emuLog(" [ INF ]   Writing %d (=0x%016llX<0x%016llX) to GPR[%d]\n", gpr[t], gpr[s], k, t);
}

void instrDMFC0(u32 instr) {
	char t = (instr >> 16) & 0x1F;
	char d = (instr >> 11) & 0x1F;
	emuLog(" [ INF ] Executing: DMFC00 %02d, %02d [PC=0x%016llX]\n", t, d, pc - 4);
	emuLog(" [ INF ]   Writing 0x%016llX from CP0R[%d] to GPRR[%d]\n", cop0Reg[d], t, d);
	gpr[t] = cop0Reg[d];
}

void instrSD(u32 instr) {
	char b = (instr >> 21) & 0x1F;
	char t = (instr >> 16) & 0x1F;
	i16 f = instr & 0xFFFF;
	u32 addr = gpr[b] + f;
	emuLog(" [ INF ] Executing: SD %02d, 0x%04X [PC=0x%016llX]\n", t, f, pc - 4);
	emuLog(" [ INF ]   Writing 0x%016llX from GPR[%d] to 0x%016llX\n", gpr[t], t, addr);
	writeu64(addr, gpr[t]);
}

void instrBLEZ(u32 instr) {
	char s = (instr >> 21) & 0x1F;
	i64 f = (i64)s16ext64(instr & 0xFFFF);
	delaySlot = pc + 4 * f;
	branchDecision = ((i64)gpr[s]) <= 0;
	delayQueue = 2;
	emuLog(" [ INF ] Executing: BLEZ %02d, %d [PC=0x%016llX]\n", s, f, pc - 4);
	emuLog(" [ INF ]   Writing 0x%016llX to Delay Slot (Condition: %d | 0x%016llX <= 0)\n", delaySlot, branchDecision, gpr[s]);
}



void instrADDU(u32 instr) {
	char s = (instr >> 21) & 0x1F;
	char t = (instr >> 16) & 0x1F;
	char d = (instr >> 11) & 0x1F;
	u64 r = s32ext64((u32)(gpr[s] + gpr[t]));
	emuLog(" [ INF ] Executing: ADDU %02d, %02d, %02d [PC=0x%016llX]\n", d, s, t, pc - 4);
	emuLog(" [ INF ]   Writing 0x%016llX (=0x%016llX+0x%016llX) to GPR[%d]\n", r, gpr[s], gpr[t], d);
	gpr[d] = r;
}

void instrADD(u32 instr) {
	char s = (instr >> 21) & 0x1F;
	char t = (instr >> 16) & 0x1F;
	char d = (instr >> 11) & 0x1F;
	u64 r = s32ext64((u32)(gpr[s] + gpr[t]));
	emuLog(" [ INF ] Executing: ADD %02d, %02d, %02d [PC=0x%016llX]\n", d, s, t, pc - 4);
	emuLog(" [ INF ]   Writing 0x%016llX (=0x%016llX+0x%016llX) to GPR[%d]\n", r, gpr[s], gpr[t], d);
	gpr[d] = r;
}

void instrSUBU(u32 instr) {
	char s = (instr >> 21) & 0x1F;
	char t = (instr >> 16) & 0x1F;
	char d = (instr >> 11) & 0x1F;
	u64 r = s32ext64((u32)(gpr[s] - gpr[t]));
	emuLog(" [ INF ] Executing: SUBU %02d, %02d, %02d [PC=0x%016llX]\n", d, s, t, pc - 4);
	emuLog(" [ INF ]   Writing 0x%016llX (=0x%016llX-0x%016llX) to GPR[%d]\n", r, gpr[s], gpr[t], d);
	gpr[d] = r;
}

void instrSUB(u32 instr) {
	char s = (instr >> 21) & 0x1F;
	char t = (instr >> 16) & 0x1F;
	char d = (instr >> 11) & 0x1F;
	u64 r = s32ext64((u32)(gpr[s] - gpr[t]));
	emuLog(" [ INF ] Executing: SUB %02d, %02d, %02d [PC=0x%016llX]\n", d, s, t, pc - 4);
	emuLog(" [ INF ]   Writing 0x%016llX (=0x%016llX-0x%016llX) to GPR[%d]\n", r, gpr[s], gpr[t], d);
	gpr[d] = r;
}

void instrMULT(u32 instr) {
	char s = (instr >> 21) & 0x1F;
	char t = (instr >> 16) & 0x1F;
	u64 r = ((i64)s32ext64(gpr[s] & 0xFFFFFFFF)) * ((i64)s32ext64(gpr[t] & 0xFFFFFFFF));
	hiReg = s32ext64((r >> 32) & 0xFFFFFFFF);
	loReg = s32ext64(r & 0xFFFFFFFF);
	emuLog(" [ INF ] Executing: MULT %02d, %02d [PC=0x%016llX]\n", s, t, pc - 4);
	emuLog(" [ INF ]   Writing 0x%016llX to HI, 0x%016llX to LO (=0x%016llX*0x%016llX)\n", hiReg, loReg, gpr[s], gpr[t]);
}

void instrMULTU(u32 instr) {
	char s = (instr >> 21) & 0x1F;
	char t = (instr >> 16) & 0x1F;
	u64 r = (gpr[s] & 0xFFFFFFFF) * (gpr[t] & 0xFFFFFFFF);
	hiReg = s32ext64((r >> 32) & 0xFFFFFFFF);
	loReg = s32ext64(r & 0xFFFFFFFF);
	emuLog(" [ INF ] Executing: MULTU %02d, %02d [PC=0x%016llX]\n", s, t, pc - 4);
	emuLog(" [ INF ]   Writing 0x%016llX to HI, 0x%016llX to LO (=0x%016llX*0x%016llX)\n", hiReg, loReg, gpr[s], gpr[t]);
}

void instrOR(u32 instr) {
	char s = (instr >> 21) & 0x1F;
	char t = (instr >> 16) & 0x1F;
	char d = (instr >> 11) & 0x1F;
	u64 r = gpr[s] | gpr[t];
	emuLog(" [ INF ] Executing: OR %02d, %02d, %02d [PC=0x%016llX]\n", d, s, t, pc - 4);
	emuLog(" [ INF ]   Writing 0x%016llX (=0x%016llX|0x%016llX) to GPR[%d]\n", r, gpr[s], gpr[t], d);
	gpr[d] = r;
}

void instrXOR(u32 instr) {
	char s = (instr >> 21) & 0x1F;
	char t = (instr >> 16) & 0x1F;
	char d = (instr >> 11) & 0x1F;
	u64 r = gpr[s] ^ gpr[t];
	emuLog(" [ INF ] Executing: XOR %02d, %02d, %02d [PC=0x%016llX]\n", d, s, t, pc - 4);
	emuLog(" [ INF ]   Writing 0x%016llX (=0x%016llX^0x%016llX) to GPR[%d]\n", r, gpr[s], gpr[t], d);
	gpr[d] = r;
}

void instrAND(u32 instr) {
	char s = (instr >> 21) & 0x1F;
	char t = (instr >> 16) & 0x1F;
	char d = (instr >> 11) & 0x1F;
	u64 r = gpr[s] & gpr[t];
	emuLog(" [ INF ] Executing: AND %02d, %02d, %02d [PC=0x%016llX]\n", d, s, t, pc - 4);
	emuLog(" [ INF ]   Writing 0x%016llX (=0x%016llX&0x%016llX) to GPR[%d]\n", r, gpr[s], gpr[t], d);
	gpr[d] = r;
}

void instrSRL(u32 instr) {
	char k = (instr >> 6) & 0x1F;
	char d = (instr >> 11) & 0x1F;
	char t = (instr >> 16) & 0x1F;
	u64 r = s32ext64(((u32)gpr[t]) >> k);
	emuLog(" [ INF ] Executing: SRL %02d, %02d, %02d [PC=0x%016llX]\n", d, t, k, pc - 4);
	emuLog(" [ INF ]   Writing 0x%016llX (=0x%016llX>>%d) to GPR[%d]\n", r, gpr[t], k, d);
	gpr[d] = r;
}

void instrSRLV(u32 instr) {
	char s = (instr >> 21) & 0x1F;
	char t = (instr >> 16) & 0x1F;
	char d = (instr >> 11) & 0x1F;
	u64 r = s32ext64(((u32)gpr[t]) >> (gpr[s] & 0x1F));
	emuLog(" [ INF ] Executing: SRLV %02d, %02d, %02d [PC=0x%016llX]\n", d, t, s, pc - 4);
	emuLog(" [ INF ]   Writing 0x%016llX (=0x%016llX>>%d) to GPR[%d]\n", r, gpr[t], (gpr[s] & 0x1F), d);
	gpr[d] = r;
}

void instrSLL(u32 instr) {
	char k = (instr >> 6) & 0x1F;
	char d = (instr >> 11) & 0x1F;
	char t = (instr >> 16) & 0x1F;
	u64 r = s32ext64(((u32)gpr[t]) << k);
	emuLog(" [ INF ] Executing: SLL %02d, %02d, %02d [PC=0x%016llX]\n", d, t, k, pc - 4);
	emuLog(" [ INF ]   Writing 0x%016llX (=0x%016llX<<%d) to GPR[%d]\n", r, gpr[t], k, d);
	gpr[d] = r;
}

void instrSLLV(u32 instr) {
	char s = (instr >> 21) & 0x1F;
	char t = (instr >> 16) & 0x1F;
	char d = (instr >> 11) & 0x1F;
	u64 r = s32ext64(((u32)gpr[t]) << (gpr[s] & 0x1F));
	emuLog(" [ INF ] Executing: SLLV %02d, %02d, %02d [PC=0x%016llX]\n", d, t, s, pc - 4);
	emuLog(" [ INF ]   Writing 0x%016llX (=0x%016llX<<%d) to GPR[%d]\n", r, gpr[t], (gpr[s] & 0x1F), d);
	gpr[d] = r;
}

void instrSLT(u32 instr) {
	char s = (instr >> 21) & 0x1F;
	char t = (instr >> 16) & 0x1F;
	char d = (instr >> 11) & 0x1F;
	gpr[d] = ((i64)gpr[s]) < ((i64)gpr[t]);
	emuLog(" [ INF ] Executing: SLT %02d, %02d, %02d [PC=0x%016llX]\n", d, s, t, pc - 4);
	emuLog(" [ INF ]   Writing %d (=0x%016llX<0x%016llX) to GPR[%d]\n", gpr[d], gpr[s], gpr[t], d);
}

void instrSLTU(u32 instr) {
	char s = (instr >> 21) & 0x1F;
	char t = (instr >> 16) & 0x1F;
	char d = (instr >> 11) & 0x1F;
	gpr[d] = gpr[s] < gpr[t];
	emuLog(" [ INF ] Executing: SLTU %02d, %02d, %02d [PC=0x%016llX]\n", d, s, t, pc - 4);
	emuLog(" [ INF ]   Writing %d (=0x%016llX<0x%016llX) to GPR[%d]\n", gpr[d], gpr[s], gpr[t], d);
}

void instrMFLO(u32 instr) {
	char d = (instr >> 11) & 0x1F;
	emuLog(" [ INF ] Executing: MFLO %02d [PC=0x%016llX]\n", d, pc - 4);
	emuLog(" [ INF ]   Writing 0x%016llX from LO to GPR[%d]\n", loReg, d);
	gpr[d] = loReg;
}

void instrJALR(u32 instr) {
	char s = (instr >> 21) & 0x1F;
	char d = (instr >> 11) & 0x1F;
	delaySlot = gpr[s];
	delayQueue = 2;
	branchDecision = 1;
	emuLog(" [ INF ] Executing: JALR %d, %d [PC=0x%016llX]\n", d, s, pc - 4);
	emuLog(" [ INF ]   Writing 0x%016llX to Delay Slot, 0x%016llX to GPR[%d]\n", delaySlot, pc + 4, d);
	gpr[d] = pc + 4;
}

void instrDADD(u32 instr) {
	char s = (instr >> 21) & 0x1F;
	char t = (instr >> 16) & 0x1F;
	char d = (instr >> 11) & 0x1F;
	u64 r = gpr[s] + gpr[t];
	emuLog(" [ INF ] Executing: DADD %02d, %02d, %02d [PC=0x%016llX]\n", d, s, t, pc - 4);
	emuLog(" [ INF ]   Writing 0x%016llX (=0x%016llX+0x%016llX) to GPR[%d]\n", r, gpr[s], gpr[t], d);
	gpr[d] = r;
}

void instrDADDU(u32 instr) {
	char s = (instr >> 21) & 0x1F;
	char t = (instr >> 16) & 0x1F;
	char d = (instr >> 11) & 0x1F;
	u64 r = gpr[s] + gpr[t];
	emuLog(" [ INF ] Executing: DADD %02d, %02d, %02d [PC=0x%016llX]\n", d, s, t, pc - 4);
	emuLog(" [ INF ]   Writing 0x%016llX (=0x%016llX+0x%016llX) to GPR[%d]\n", r, gpr[s], gpr[t], d);
	gpr[d] = r;
}

void instrDDIV(u32 instr) {
	char s = (instr >> 21) & 0x1F;
	char t = (instr >> 16) & 0x1F;
	if (gpr[t] == 0) {
		loReg = ((i64)gpr[s] < 0) ? 1 : ULLONG_MAX;
		hiReg = gpr[s];
	}
	else if (((i64)gpr[s]) == LLONG_MIN && ((i64)gpr[t]) == -1) {
		loReg = gpr[s];
		hiReg = 0;
	}
	else {
		loReg = ((i64)gpr[s]) / ((i64)gpr[t]);
		hiReg = ((i64)gpr[s]) % ((i64)gpr[t]);
	}
	emuLog(" [ INF ] Executing: DDIV %02d, %02d [PC=0x%016llX]\n", s, t, pc - 4);
	emuLog(" [ INF ]   Writing 0x%016llX to HI, 0x%016llX to LO (=0x%016llX/0x%016llX)\n", hiReg, loReg, gpr[s], gpr[t]);
}

void instrDDIVU(u32 instr) {
	char s = (instr >> 21) & 0x1F;
	char t = (instr >> 16) & 0x1F;
	if (gpr[t] == 0) {
		loReg = ULLONG_MAX;
		hiReg = gpr[s];
	}
	else {
		loReg = gpr[s] / gpr[t];
		hiReg = gpr[s] % gpr[t];
	}
	emuLog(" [ INF ] Executing: DDIVU %02d, %02d [PC=0x%016llX]\n", s, t, pc - 4);
	emuLog(" [ INF ]   Writing 0x%016llX to HI, 0x%016llX to LO (=0x%016llX/0x%016llX)\n", hiReg, loReg, gpr[s], gpr[t]);
}

void instrMFHI(u32 instr) {
	char d = (instr >> 11) & 0x1F;
	emuLog(" [ INF ] Executing: MFHI %02d [PC=0x%016llX]\n", d, pc - 4);
	emuLog(" [ INF ]   Writing 0x%016llX from HI to GPR[%d]\n", hiReg, d);
	gpr[d] = hiReg;
}

void instrDIV(u32 instr) {
	char s = (instr >> 21) & 0x1F;
	char t = (instr >> 16) & 0x1F;
	if (gpr[t] == 0) {
		loReg = ((i64)gpr[s] < 0) ? 1 : ULLONG_MAX;
		hiReg = gpr[s];
	}
	else if (((i64)gpr[s]) == LLONG_MIN && ((i64)gpr[t]) == -1) {
		loReg = gpr[s];
		hiReg = 0;
	}
	else {
		loReg = ((i64)gpr[s]) / ((i64)gpr[t]);
		hiReg = ((i64)gpr[s]) % ((i64)gpr[t]);
	}
	emuLog(" [ INF ] Executing: DIV %02d, %02d [PC=0x%016llX]\n", s, t, pc - 4);
	emuLog(" [ INF ]   Writing 0x%016llX to HI, 0x%016llX to LO (=0x%016llX/0x%016llX)\n", hiReg, loReg, gpr[s], gpr[t]);
}

void instrDIVU(u32 instr) {
	char s = (instr >> 21) & 0x1F;
	char t = (instr >> 16) & 0x1F;
	if (gpr[t] == 0) {
		loReg = ULLONG_MAX;
		hiReg = s32ext64(gpr[s]);
	}
	else {
		loReg = s32ext64(gpr[s] / gpr[t]);
		hiReg = s32ext64(gpr[s] % gpr[t]);
	}
	emuLog(" [ INF ] Executing: DIVU %02d, %02d [PC=0x%016llX]\n", s, t, pc - 4);
	emuLog(" [ INF ]   Writing 0x%016llX to HI, 0x%016llX to LO (=0x%016llX/0x%016llX)\n", hiReg, loReg, gpr[s], gpr[t]);
}

void instrSRA(u32 instr) {
	char k = (instr >> 6) & 0x1F;
	char d = (instr >> 11) & 0x1F;
	char t = (instr >> 16) & 0x1F;
	u64 r = s32ext64(((i32)gpr[t]) >> k);
	emuLog(" [ INF ] Executing: SRA %02d, %02d, %02d [PC=0x%016llX]\n", d, t, k, pc - 4);
	emuLog(" [ INF ]   Writing 0x%016llX (=0x%016llX>>%d) to GPR[%d]\n", r, gpr[t], k, d);
	gpr[d] = r;
}

void instrMTLO(u32 instr) {
	char d = (instr >> 11) & 0x1F;
	emuLog(" [ INF ] Executing: MTLO %02d [PC=0x%016llX]\n", d, pc - 4);
	emuLog(" [ INF ]   Writing 0x%016llX from GPR[%d] to LO\n", loReg, d);
	loReg = gpr[d];
}

void instrMTHI(u32 instr) {
	char d = (instr >> 11) & 0x1F;
	emuLog(" [ INF ] Executing: MTHI %02d [PC=0x%016llX]\n", d, pc - 4);
	emuLog(" [ INF ]   Writing 0x%016llX from GPR[%d] to HI\n", loReg, d);
	hiReg = gpr[d];
}



void instrCFC(u32 instr) {
	char t = (instr >> 16) & 0x1F;
	char d = (instr >> 11) & 0x1F;
	gpr[t] = d == 0 ? fcr0 : fcr31;
	emuLog(" [ INF ] Executing: CFC1 %02d, %02d [PC=0x%016llX]\n", t, d, pc - 4);
	emuLog(" [ INF ]   Writing 0x%016llX from FCR%d to GPR[%d]\n", gpr[t], d, t);
}

void instrCTC(u32 instr) {
	char t = (instr >> 16) & 0x1F;
	char d = (instr >> 11) & 0x1F;
	if (d == 0)
		fcr0 = gpr[t];
	else
		setFCR31(gpr[t]);
	emuLog(" [ INF ] Executing: CTC1 %02d, %02d [PC=0x%016llX]\n", t, d, pc - 4);
	emuLog(" [ INF ]   Writing 0x%016llX from GPR[%d] to FCR%d\n", gpr[t], t, d);
}

void instrMTC1(u32 instr) {
	char t = (instr >> 16) & 0x1F;
	char s = (instr >> 11) & 0x1F;
	u32 v = (u32)gpr[t];
	setFPR_W(s, v);
}

void instrMFC1(u32 instr) {
	char t = (instr >> 16) & 0x1F;
	char s = (instr >> 11) & 0x1F;
	u32 v = getFPR_W(s);
	gpr[t] = v;
}

void instrLWC1(u32 instr) {
	char b = (instr >> 21) & 0x1F;
	char t = (instr >> 16) & 0x1F;
	i32 f = (i32)s16ext32(instr & 0xFFFF);
	u32 addr = gpr[b] + f;
	u32 w = readu32(addr);
	setFPR_W(t, w);
}

void instrLDC1(u32 instr) {
	char b = (instr >> 21) & 0x1F;
	char t = (instr >> 16) & 0x1F;
	i32 f = (i32)s16ext32(instr & 0xFFFF);
	u32 addr = gpr[b] + f;
	u64 w = readu64(addr);
	setFPR_L(t, w);
}

void instrSDC1(u32 instr) {
	char b = (instr >> 21) & 0x1F;
	char t = (instr >> 16) & 0x1F;
	i32 f = (i32)s16ext32(instr & 0xFFFF);
	u32 addr = gpr[b] + f;
	writeu64(addr, getFPR_L(t));
}

void instrSWC1(u32 instr) {
	char b = (instr >> 21) & 0x1F;
	char t = (instr >> 16) & 0x1F;
	i32 f = (i32)s16ext32(instr & 0xFFFF);
	u32 addr = gpr[b] + f;
	writeu32(addr, getFPR_W(t));
}

void instrCVT_D_L(u32 instr) {
	char s = (instr >> 11) & 0x1F;
	char d = (instr >> 6) & 0x1F;
	i64 bv = getFPR_L(s);
	double v = (double)bv;
	setFPR_D(d, v);
}

void instrCVT_D_S(u32 instr) {
	char s = (instr >> 11) & 0x1F;
	char d = (instr >> 6) & 0x1F;
	float bv = getFPR_S(s);
	double v = (double)bv;
	setFPR_D(d, v);
}

void instrCVT_D_W(u32 instr) {
	char s = (instr >> 11) & 0x1F;
	char d = (instr >> 6) & 0x1F;
	i32 bv = getFPR_W(s);
	double v = (double)bv;
	setFPR_D(d, v);
}

void instrCVT_L_D(u32 instr) {
	char s = (instr >> 11) & 0x1F;
	char d = (instr >> 6) & 0x1F;
	double bv = getFPR_D(s);
	u64 v = (u64)bv;
	setFPR_L(d, v);
}

void instrCVT_L_S(u32 instr) {
	char s = (instr >> 11) & 0x1F;
	char d = (instr >> 6) & 0x1F;
	float bv = getFPR_S(s);
	u64 v = (u64)bv;
	setFPR_L(d, v);
}

void instrCVT_S_D(u32 instr) {
	char s = (instr >> 11) & 0x1F;
	char d = (instr >> 6) & 0x1F;
	double bv = getFPR_D(s);
	float v = (float)bv;
	setFPR_S(d, v);
}

void instrCVT_S_L(u32 instr) {
	char s = (instr >> 11) & 0x1F;
	char d = (instr >> 6) & 0x1F;
	i64 bv = getFPR_L(s);
	float v = (float)bv;
	setFPR_S(d, v);
}

void instrCVT_S_W(u32 instr) {
	char s = (instr >> 11) & 0x1F;
	char d = (instr >> 6) & 0x1F;
	i32 bv = getFPR_W(s);
	float v = (float)bv;
	setFPR_S(d, v);
}

void instrCVT_W_D(u32 instr) {
	char s = (instr >> 11) & 0x1F;
	char d = (instr >> 6) & 0x1F;
	double bv = getFPR_D(s);
	u32 v = (u32)bv;
	setFPR_W(d, v);
}

void instrCVT_W_S(u32 instr) {
	char s = (instr >> 11) & 0x1F;
	char d = (instr >> 6) & 0x1F;
	float bv = getFPR_S(s);
	u32 v = (u32)bv;
	setFPR_W(d, v);
}



void instrDIV_S(u32 instr) {
	char t = (instr >> 16) & 0x1F;
	char s = (instr >> 11) & 0x1F;
	char d = (instr >> 6) & 0x1F;
	float v1 = getFPR_S(s);
	float v2 = getFPR_S(t);
	setFPR_S(d, v1 / v2);
}

void instrDIV_D(u32 instr) {
	char t = (instr >> 16) & 0x1F;
	char s = (instr >> 11) & 0x1F;
	char d = (instr >> 6) & 0x1F;
	double v1 = getFPR_D(s);
	double v2 = getFPR_D(t);
	setFPR_D(d, v1 / v2);
}



void instrADD_S(u32 instr) {
	char t = (instr >> 16) & 0x1F;
	char s = (instr >> 11) & 0x1F;
	char d = (instr >> 6) & 0x1F;
	float v1 = getFPR_S(s);
	float v2 = getFPR_S(t);
	setFPR_S(d, v1 + v2);
}

void instrADD_D(u32 instr) {
	char t = (instr >> 16) & 0x1F;
	char s = (instr >> 11) & 0x1F;
	char d = (instr >> 6) & 0x1F;
	double v1 = getFPR_D(s);
	double v2 = getFPR_D(t);
	setFPR_D(d, v1 + v2);
}



void instrTRUNC_L_D(u32 instr) {
	char s = (instr >> 11) & 0x1F;
	char d = (instr >> 6) & 0x1F;
	double bv = getFPR_D(s);
	u64 v = (u64)bv;
	setFPR_L(d, v);
}

void instrTRUNC_L_S(u32 instr) {
	char s = (instr >> 11) & 0x1F;
	char d = (instr >> 6) & 0x1F;
	float bv = getFPR_S(s);
	u64 v = (u64)bv;
	setFPR_L(d, v);
}

void instrTRUNC_W_D(u32 instr) {
	char s = (instr >> 11) & 0x1F;
	char d = (instr >> 6) & 0x1F;
	double bv = getFPR_D(s);
	u32 v = (u32)bv;
	setFPR_W(d, v);
}

void instrTRUNC_W_S(u32 instr) {
	char s = (instr >> 11) & 0x1F;
	char d = (instr >> 6) & 0x1F;
	float bv = getFPR_S(s);
	u32 v = (u32)bv;
	setFPR_W(d, v);
}



void instrC_LE_S(u32 instr) {
	char t = (instr >> 16) & 0x1F;
	char s = (instr >> 11) & 0x1F;
	float v1 = getFPR_S(s);
	float v2 = getFPR_S(t);
	if (v1 <= v2)
		setFCR31(fcr31 | (u32)(1 << 23));
	else
		setFCR31(fcr31 & ~((u32)(1 << 23)));
}

void instrC_LE_D(u32 instr) {
	char t = (instr >> 16) & 0x1F;
	char s = (instr >> 11) & 0x1F;
	double v1 = getFPR_D(s);
	double v2 = getFPR_D(t);
	if (v1 <= v2)
		setFCR31(fcr31 | (u32)(1 << 23));
	else
		setFCR31(fcr31 & ~((u32)(1 << 23)));
}



void instrBC1T(u32 instr) {
	i64 f = (i64)s16ext64(instr & 0xFFFF);
	delaySlot = pc + 4 * f;
	branchDecision = (fcr31 & (1 << 23));
	delayQueue = 2;
	emuLog(" [ INF ] Executing: BC1T %d [PC=0x%016llX]\n", f, pc - 4);
	emuLog(" [ INF ]   Writing 0x%016llX to Delay Slot (Condition: %d)\n", delaySlot, branchDecision);
}

void instrBC1TL(u32 instr) {
	i64 f = (i64)s16ext64(instr & 0xFFFF);
	delaySlot = pc + 4 * f;
	branchDecision = (fcr31 & (1 << 23));
	delayQueue = 2;
	if (!branchDecision)
		pc += 4;
	emuLog(" [ INF ] Executing: BC1TL %d [PC=0x%016llX]\n", f, pc - 4);
	emuLog(" [ INF ]   Writing 0x%016llX to Delay Slot (Condition: %d)\n", delaySlot, branchDecision);
}

void instrBC1F(u32 instr) {
	i64 f = (i64)s16ext64(instr & 0xFFFF);
	delaySlot = pc + 4 * f;
	branchDecision = (fcr31 & (1 << 23)) == 0;
	delayQueue = 2;
	emuLog(" [ INF ] Executing: BC1T %d [PC=0x%016llX]\n", f, pc - 4);
	emuLog(" [ INF ]   Writing 0x%016llX to Delay Slot (Condition: %d)\n", delaySlot, branchDecision);
}



void instrTLBWI(u32 instr) {
	emuLog(" [ WRN ] TLBWI Instruction encountered and ignored. [PC=0x%016llX]\n", pc-4);
}

void instrERET(u32 instr) {
	emuLog(" [ INF ] Executing: ERET [PC=0x%016llX]\n", pc - 4);
	if (cop0Reg[CP0R_Status] & 0b100) {
		pc = cop0Reg[CP0R_ErrorEPC];
		cop0Reg[CP0R_Status] &= (~0b100);
		emuLog(" [ INF ]   Writing 0x%016llX from CP0R_ErrorEPC to PC\n", pc);
	} else {
		pc = cop0Reg[CP0R_EPC];
		cop0Reg[CP0R_Status] &= (~0b10);
		emuLog(" [ INF ]   Writing 0x%016llX from CP0R_EPC to PC\n", pc);
	}
}