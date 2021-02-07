#ifndef __CPU_DECODE_H__
#define __CPU_DECODE_H__

#include <isa.h>

#define OP_STR_SIZE 40
enum { OP_TYPE_REG, OP_TYPE_MEM, OP_TYPE_IMM };

typedef struct {
//  uint32_t type;
//  int width;
  union {
    rtlreg_t *preg;
    word_t imm;
    sword_t simm;
  };
//  rtlreg_t val;
  IFDEF(CONFIG_DEBUG, int reg);
  IFDEF(CONFIG_DEBUG, char str[OP_STR_SIZE]);
} Operand;

typedef union DecodeExecState {
  struct {
    union DecodeExecState *next;
    vaddr_t pc;
    vaddr_t snpc; // sequential next pc
    const void *EHelper;
    Operand dest, src1, src2;
    ISADecodeInfo isa;
  };
  uint8_t pad[64];
} DecodeExecState;

#define def_DHelper(name) void concat(decode_, name) (DecodeExecState *s)
#define print_Dop(...) IFDEF(CONFIG_DEBUG, snprintf(__VA_ARGS__))

#endif
