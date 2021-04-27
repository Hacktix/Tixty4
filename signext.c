#include "signext.h"

u64 s32ext64(u32 val) { return (val & 0xFFFFFFFF) | ((val & 0x80000000) ? 0xFFFFFFFF00000000 : 0); }
u32 s16ext32(u32 val) { return (val & 0xFFFF) | ((val & 0x8000) ? 0xFFFF0000 : 0); }