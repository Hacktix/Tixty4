#pragma once
#include "main.h"
#include <stdlib.h>

#define GPR_ZERO 0
#define GPR_AT 1
#define GPR_V0 2
#define GPR_V1 3
#define GPR_A0 4
#define GPR_A1 5
#define GPR_A2 6
#define GPR_A3 7
#define GPR_T0 8
#define GPR_T1 9
#define GPR_T2 10
#define GPR_T3 11
#define GPR_T4 12
#define GPR_T5 13
#define GPR_T6 14
#define GPR_T7 15
#define GPR_S0 16
#define GPR_S1 17
#define GPR_S2 18
#define GPR_S3 19
#define GPR_S4 20
#define GPR_S5 21
#define GPR_S6 22
#define GPR_S7 23
#define GPR_T8 24
#define GPR_T9 25
#define GPR_K0 26
#define GPR_K1 27
#define GPR_GP 28
#define GPR_SP 29
#define GPR_S8 30
#define GPR_RA 31

#define CP0R_Index 0
#define CP0R_Random 1
#define CP0R_EntryLo0 2
#define CP0R_EntryLo1 3
#define CP0R_Context 4
#define CP0R_PageMask 5
#define CP0R_Wired 6
#define CP0R_7 7
#define CP0R_BadVAddr 8
#define CP0R_Count 9
#define CP0R_EntryHi 10
#define CP0R_Compare 11
#define CP0R_Status 12
#define CP0R_Cause 13
#define CP0R_EPC 14
#define CP0R_PRId 15
#define CP0R_Config 16
#define CP0R_LLAddr 17
#define CP0R_WatchLo 18
#define CP0R_WatchHi 19
#define CP0R_XContext 20
#define CP0R_21 21
#define CP0R_22 22
#define CP0R_23 23
#define CP0R_24 24
#define CP0R_25 25
#define CP0R_ParityError 26
#define CP0R_CacheError 27
#define CP0R_TagLo 28
#define CP0R_TagHi 29
#define CP0R_ErrorEPC 30
#define CP0R_31 31

u64* gpr;
u64* cop0Reg;
u64 pc;

u64 hiReg;
u64 loReg;

u32 fcr0;
u32 fcr31;

long long delaySlot;
int delayQueue;
int branchDecision;

int cpuInit();
void cpuInitPIF();

int cpuExec();

void instrMTC0(u32 instr);
void instrLUI(u32 instr);
void instrADDIU(u32 instr);
void instrLW(u32 instr);
void instrBNE(u32 instr);
void instrSW(u32 instr);
void instrORI(u32 instr);
void instrADDI(u32 instr);
void instrJAL(u32 instr);
void instrSLTI(u32 instr);
void instrBEQL(u32 instr);
void instrJR(u32 instr);
void instrANDI(u32 instr);
void instrXORI(u32 instr);
void instrBEQ(u32 instr);
void instrCACHE(u32 instr);
void instrBNEL(u32 instr);
void instrBLEZL(u32 instr);
void instrSB(u32 instr);
void instrLBU(u32 instr);
void instrBGEZL(u32 instr);
void instrJ(u32 instr);
void instrLB(u32 instr);
void instrBGTZ(u32 instr);
void instrLWU(u32 instr);
void instrDADDI(u32 instr);
void instrLHU(u32 instr);
void instrLD(u32 instr);
void instrDADDIU(u32 instr);
void instrSLTIU(u32 instr);
void instrDMFC0(u32 instr);
void instrSD(u32 instr);
void instrBLEZ(u32 instr);
void instrSH(u32 instr);
void instrLH(u32 instr);

void instrADDU(u32 instr);
void instrADD(u32 instr);
void instrSUBU(u32 instr);
void instrSUB(u32 instr);
void instrMULT(u32 instr);
void instrMULTU(u32 instr);
void instrOR(u32 instr);
void instrXOR(u32 instr);
void instrAND(u32 instr);
void instrSRL(u32 instr);
void instrSRLV(u32 instr);
void instrSLL(u32 instr);
void instrSLLV(u32 instr);
void instrSLT(u32 instr);
void instrSLTU(u32 instr);
void instrMFLO(u32 instr);
void instrJALR(u32 instr);
void instrDADD(u32 instr);
void instrDADDU(u32 instr);
void instrDDIV(u32 instr);
void instrDDIVU(u32 instr);
void instrMFHI(u32 instr);
void instrDIV(u32 instr);
void instrDIVU(u32 instr);
void instrSRA(u32 instr);
void instrMTLO(u32 instr);
void instrMTHI(u32 instr);

void instrCFC(u32 instr);
void instrCTC(u32 instr);

void instrTLBWI(u32 instr);