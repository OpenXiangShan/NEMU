#ifndef __SDI_H__
#define __SDI_H__

#include <common.h>

extern volatile uint8_t *tl_base;
#define TL_MMIO_BASE 0x10000000
#define TL_A_VALID 0
#define TL_A_ADDR 4
#define TL_A_WAY 8
#define TL_D_VALID 12
#define TL_D_DATA 16
extern volatile uint8_t *jmp_base;
#define JMP_MMIO_BASE 0x10001000
#define JMP_VALID 0
#define JMP_SPC 4
#define JMP_TARGET 8

#endif
