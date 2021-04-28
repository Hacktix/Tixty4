#pragma once
#include <stdio.h>
#include <stdlib.h>

int hitDbgBrk;
int triggerDbgBrk;

void emuStart(FILE* romf);
void emuLog(const char* fmt, va_list argp);