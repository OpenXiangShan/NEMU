/***************************************************************************************
* Copyright (c) 2026 Institute of Computing Technology, Chinese Academy of Sciences
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

// The interpreter's instruction helpers compiled a second time as ordinary
// functions, so translated code can execute any straight-line instruction
// with exactly the interpreter's semantics.

#include <cpu/cpu.h>
#include <cpu/exec.h>
#include <cpu/difftest.h>
#include <cpu/decode.h>
#include <isa-all-instr.h>
#include <generated/autoconf.h>
#ifdef CONFIG_RV_ZICFILP
#include "../isa/riscv64/local-include/intr.h"
#endif
#include <profiling/profiling_control.h>
#include <checkpoint/semantic_point.h>
#include "../local-include/trigger.h"
#include "../local-include/aia.h"
#include "jit-internal.h"

// Control transfers and privileged sequencing never run through here; the
// translator only routes straight-line instructions to these functions.
static void jit_eh_forbidden(Decode *s) {
  panic("JIT: control-transfer helper called for pc " FMT_WORD, s->pc);
}

#undef def_EHelper
#define def_EHelper(name) static void concat(jit_eh_, name)(Decode *s)
#define rtl_j(s, target)                         jit_eh_forbidden(s)
#define rtl_jr(s, target)                        jit_eh_forbidden(s)
#define rtl_jrelop(s, relop, src1, src2, target) jit_eh_forbidden(s)
#define rtl_priv_next(s)                         jit_eh_forbidden(s)
#define rtl_priv_jr(s, target)                   jit_eh_forbidden(s)

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#include "isa-exec.h"
#pragma GCC diagnostic pop

#define JIT_EH_ENTRY(name) [concat(EXEC_ID_, name)] = concat(jit_eh_, name),
static void (*const jit_eh_table[TOTAL_INSTR])(Decode *) = {
  MAP(INSTR_LIST, JIT_EH_ENTRY)
};

void (*jit_eh_lookup(int exec_id))(Decode *) {
  return jit_eh_table[exec_id];
}
