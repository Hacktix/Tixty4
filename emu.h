#pragma once
#include <stdio.h>
#include <stdlib.h>

char* rom;
int rom_size;

void loadRom(FILE* romf);
void byteswapRom();