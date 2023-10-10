/***************************************************************************************
* Copyright (c) 2014-2021 Zihao Yu, Nanjing University
* Copyright (c) 2020-2022 Institute of Computing Technology, Chinese Academy of Sciences
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#ifndef __CPU_DECODE_H__
#define __CPU_DECODE_H__

#include <isa.h>

#define OP_STR_SIZE 40

typedef struct {
  union {
    IFDEF(CONFIG_ISA_x86, uint64_t *pfreg);
    IFDEF(CONFIG_ISA_x86, uint64_t fval);
    rtlreg_t *preg;
    word_t imm;
    sword_t simm;
  };
  IFDEF(CONFIG_ISA_x86, rtlreg_t val);
  IFDEF(CONFIG_ISA_x86, uint8_t type);
  IFDEF(CONFIG_ISA_x86, uint8_t reg);
  IFDEF(CONFIG_RVV, rtlreg_t val);
  IFDEF(CONFIG_RVV, uint8_t reg);
  IFDEF(CONFIG_DEBUG, char str[OP_STR_SIZE]);
} Operand;

enum {
  INSTR_TYPE_N, // normal
  INSTR_TYPE_J, // jump
  INSTR_TYPE_B, // branch
  INSTR_TYPE_I, // indirect
};

typedef struct Decode {
  union {
    struct {
      struct Decode *tnext;  // next pointer for taken branch and jump
      struct Decode *ntnext; // next pointer for non-taken branch
    };
    struct {  // only used by tcache_tmp_pool
      struct Decode *list_next; // next pointer for list
      struct Decode *bb_src;    // pointer recording the source of basic block direction
    };
  };
  vaddr_t pc;
  vaddr_t snpc; // sequential next pc
  IFDEF (CONFIG_PERF_OPT, const void *EHelper);
  IFNDEF(CONFIG_PERF_OPT, void (*EHelper)(struct Decode *));
  Operand dest, src1, src2;
  vaddr_t jnpc;
  uint16_t idx_in_bb; // the number of instruction in the basic block, start from 1
  uint8_t type;
  ISADecodeInfo isa;
  IFDEF(CONFIG_DEBUG, char logbuf[80]);
  #ifdef CONFIG_RVV
  // for vector
  int v_width;
  int v_nf;
  int v_lsumop;
  uint32_t vm;
  uint32_t src_vmode;
  rtlreg_t tmp_reg[4];
  #endif // CONFIG_RVV

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


#define def_DHelper(name) void concat(decode_, name) (Decode *s, int width)
// empty decode helper
static inline def_DHelper(empty) {}

#define CASE_ENTRY(idx, id, tab) case idx: id(s); return tab(s);
#define IDTAB(idx, id, tab) CASE_ENTRY(idx, concat(decode_, id), concat(table_, tab))
#define TAB(idx, tab) IDTAB(idx, empty, tab)
#define EMPTY(idx) TAB(idx, inv)

__attribute__((always_inline))
static inline void pattern_decode(const char *str, int len,
    uint64_t *key, uint64_t *mask, uint64_t *shift) {
  uint64_t __key = 0, __mask = 0, __shift = 0;
#define macro(i) \
  if ((i) >= len) goto finish; \
  else { \
    char c = str[i]; \
    if (c != ' ') { \
      Assert(c == '0' || c == '1' || c == '?', \
          "invalid character '%c' in pattern string", c); \
      __key  = (__key  << 1) | (c == '1' ? 1 : 0); \
      __mask = (__mask << 1) | (c == '?' ? 0 : 1); \
      __shift = (c == '?' ? __shift + 1 : 0); \
    } \
  }

#define macro2(i)  macro(i);   macro((i) + 1)
#define macro4(i)  macro2(i);  macro2((i) + 2)
#define macro8(i)  macro4(i);  macro4((i) + 4)
#define macro16(i) macro8(i);  macro8((i) + 8)
#define macro32(i) macro16(i); macro16((i) + 16)
#define macro64(i) macro32(i); macro32((i) + 32)
  macro64(0);
#undef macro
finish:
  *key = __key >> __shift;
  *mask = __mask >> __shift;
  *shift = __shift;
}

#define def_INSTR_raw(pattern, body) do { \
  uint64_t key, mask, shift; \
  pattern_decode(pattern, STRLEN(pattern), &key, &mask, &shift); \
  if ((((uint64_t)get_instr(s) >> shift) & mask) == key) { body; } \
} while (0)

#define def_INSTR_IDTABW(pattern, id, tab, width) \
  def_INSTR_raw(pattern, { concat(decode_, id)(s, width); return concat(table_, tab)(s); })

#define def_INSTR_IDTAB(pattern, id, tab)   def_INSTR_IDTABW(pattern, id, tab, 0)
#define def_INSTR_TABW(pattern, tab, width) def_INSTR_IDTABW(pattern, empty, tab, width)
#define def_INSTR_TAB(pattern, tab)         def_INSTR_IDTABW(pattern, empty, tab, 0)


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
