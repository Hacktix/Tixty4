#pragma once
#include "main.h"
#include <stdio.h>
#include <stdlib.h>

u8* rom;
int rom_size;

u8* RDRAM;
u8* SPDmem;
u8* SPImem;
u8* RIreg;
u8* MIreg;
u8* RDRAMreg;
u8* PIreg;
u8* SPreg;
u8* SIreg;
u8* AIreg;
u8* VIreg;

u8* PIFram;

int mmuInit(FILE* romf);

u8 readu8(u64 vaddr);
u16 readu16(u64 vaddr);
u32 readu32(u64 vaddr);
u64 readu64(u64 vaddr);

i8 readi8(u64 vaddr);
i16 readi16(u64 vaddr);
i32 readi32(u64 vaddr);
i64 readi64(u64 vaddr);

void writeu8(u64 vaddr, u8 val);
void writeu16(u64 vaddr, u16 val);
void writeu32(u64 vaddr, u32 val);
void writeu64(u64 vaddr, u64 val);

void writei8(u64 vaddr, i8 val);
void writei16(u64 vaddr, i16 val);
void writei32(u64 vaddr, i32 val);
void writei64(u64 vaddr, i64 val);

u8 readPhys(u64 paddr);
void writePhys(u64 paddr, u8 val);

void byteswapRom();

u32 getu32(u32* ptr);