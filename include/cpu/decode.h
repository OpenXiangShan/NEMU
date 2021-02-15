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

#define def_THelper(name) \
  static inline int concat(table_, name) (DecodeExecState *s)

#define def_EXEC_ID(name) \
  enum { concat(EXEC_ID_, name) = __COUNTER__ }; \
  def_THelper(name) { return concat(EXEC_ID_, name); }
#define def_all_EXEC_ID() MAP(INSTR_LIST, def_EXEC_ID)

#define INSTR_CNT(name) + 1
#define TOTAL_INSTR (0 MAP(INSTR_LIST, INSTR_CNT))

#define FILL_JMP_TABLE(name) [concat(EXEC_ID_, name)] = &&name,
#define def_jmp_table() \
    static const void* jmp_table[TOTAL_INSTR] = { MAP(INSTR_LIST, FILL_JMP_TABLE) };

#define def_DHelper(name) void concat(decode_, name) (DecodeExecState *s)
// empty decode helper
static inline def_DHelper(empty) {}

#define CASE_ENTRY(idx, id, tab) case idx: id(s); return tab(s);
#define IDTAB(idx, id, tab) CASE_ENTRY(idx, concat(decode_, id), concat(table_, tab))
#define TAB(idx, tab) IDTAB(idx, empty, tab)
#define EMPTY(idx) TAB(idx, inv)

#define print_Dop(...) IFDEF(CONFIG_DEBUG, snprintf(__VA_ARGS__))

#endif
