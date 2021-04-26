#pragma once
#include "main.h"
#include <stdio.h>
#include <stdlib.h>

u8* rom;
int rom_size;

u8* SPDmem;

int mmuInit(FILE* romf);

u8 readu8(long long vaddr);
u16 readu16(long long vaddr);
u32 readu32(long long vaddr);
u64 readu64(long long vaddr);

i8 readi8(long long vaddr);
i16 readi16(long long vaddr);
i32 readi32(long long vaddr);
i64 readi64(long long vaddr);

void writeu8(long long vaddr, u8 val);
void writeu16(long long vaddr, u16 val);
void writeu32(long long vaddr, u32 val);
void writeu64(long long vaddr, u64 val);

void writei8(long long vaddr, i8 val);
void writei16(long long vaddr, i16 val);
void writei32(long long vaddr, i32 val);
void writei64(long long vaddr, i64 val);

u8 readPhys(long long paddr);
void writePhys(long long paddr, u8 val);

void byteswapRom();