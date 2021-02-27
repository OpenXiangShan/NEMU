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

enum {
  INSTR_TYPE_N, // normal
  INSTR_TYPE_J, // jump
  INSTR_TYPE_B, // branch
  INSTR_TYPE_I, // indirect
};

typedef struct Decode {
  struct Decode *tnext;  // next pointer for taken branch and jump
  struct Decode *ntnext; // next pointer for non-taken branch
  vaddr_t pc;
  vaddr_t snpc; // sequential next pc
  const void *EHelper;
  Operand dest, src1, src2;
  vaddr_t jnpc;
  uint8_t type;
  ISADecodeInfo isa;
  IFDEF(CONFIG_DEBUG, char logbuf[80]);
} Decode;

#define id_src1 (&s->src1)
#define id_src2 (&s->src2)
#define id_dest (&s->dest)


#define INSTR_LIST(f) INSTR_NULLARY(f) INSTR_UNARY(f) INSTR_BINARY(f) INSTR_TERNARY(f)

#define def_EXEC_ID(name) \
  enum { concat(EXEC_ID_, name) = __COUNTER__ };
#define def_all_EXEC_ID() MAP(INSTR_LIST, def_EXEC_ID)

#define INSTR_CNT(name) + 1
#define TOTAL_INSTR (0 MAP(INSTR_LIST, INSTR_CNT))


#define def_THelper(name) \
  static inline int concat(table_, name) (Decode *s)
#define def_THelper_arity(name, arity) \
  def_THelper(name) { concat(print_asm_template, arity)(name); return concat(EXEC_ID_, name); }
#define def_THelper_nullary(name) def_THelper_arity(name, 0)
#define def_THelper_unary(name)   def_THelper_arity(name, 1)
#define def_THelper_binary(name)  def_THelper_arity(name, 2)
#define def_THelper_ternary(name) def_THelper_arity(name, 3)

#define def_all_THelper() \
  MAP(INSTR_NULLARY, def_THelper_nullary) \
  MAP(INSTR_UNARY,   def_THelper_unary  ) \
  MAP(INSTR_BINARY,  def_THelper_binary ) \
  MAP(INSTR_TERNARY, def_THelper_ternary)


#define def_DHelper(name) void concat(decode_, name) (Decode *s)
// empty decode helper
static inline def_DHelper(empty) {}

#define CASE_ENTRY(idx, id, tab) case idx: id(s); return tab(s);
#define IDTAB(idx, id, tab) CASE_ENTRY(idx, concat(decode_, id), concat(table_, tab))
#define TAB(idx, tab) IDTAB(idx, empty, tab)
#define EMPTY(idx) TAB(idx, inv)


#define print_Dop(...) IFDEF(CONFIG_DEBUG, snprintf(__VA_ARGS__))
#define print_asm(...) IFDEF(CONFIG_DEBUG, snprintf(log_asmbuf, sizeof(log_asmbuf), __VA_ARGS__))

#ifndef suffix_char
#define suffix_char(width) ' '
#endif

#define print_asm_template0(instr) \
  print_asm(str(instr) "%c", suffix_char(id_dest->width))

#define print_asm_template1(instr) \
  print_asm(str(instr) "%c %s", suffix_char(id_dest->width), id_dest->str)

#define print_asm_template2(instr) \
  print_asm(str(instr) "%c %s,%s", suffix_char(id_dest->width), id_src1->str, id_dest->str)

#define print_asm_template3(instr) \
  print_asm(str(instr) "%c %s,%s,%s", suffix_char(id_dest->width), id_src1->str, id_src2->str, id_dest->str)

#endif
