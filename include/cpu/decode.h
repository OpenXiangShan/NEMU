#ifndef __CPU_DECODE_H__
#define __CPU_DECODE_H__

#include <isa.h>

#define OP_STR_SIZE 40
enum { OP_TYPE_REG, OP_TYPE_MEM, OP_TYPE_IMM };

typedef struct {
  uint32_t type;
  int width;
  union {
    uint32_t reg;
    word_t imm;
    sword_t simm;
  };
  rtlreg_t *preg;
  rtlreg_t val;
#ifdef DEBUG
  char str[OP_STR_SIZE];
#endif
} Operand;

typedef struct {
  vaddr_t snpc; // sequential next pc
  vaddr_t npc;  // real next pc
  uint32_t opcode;
  Operand src1, dest, src2;
  int width;
  rtlreg_t tmp_reg[4];
  ISADecodeInfo isa;
} DecodeExecState;

#define def_DHelper(name) void concat(decode_, name) (DecodeExecState *s)

#ifdef DEBUG
#define print_Dop(...) snprintf(__VA_ARGS__)
#else
#define print_Dop(...)
#endif

#endif
