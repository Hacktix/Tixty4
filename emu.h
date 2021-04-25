#pragma once
#include <stdio.h>
#include <stdlib.h>

char* rom;
int rom_size;

void emuStart();
void loadRom(FILE* romf);
void byteswapRom();