/***************************************************************************************
* Copyright (c) 2014-2021 Zihao Yu, Nanjing University
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

#ifndef __CPU_EXEC_H__
#define __CPU_EXEC_H__

#include <cpu/decode.h>

#ifdef CONFIG_PERF_OPT
#define finish_label exec_finish
#define def_label(l) l:
#ifdef CONFIG_HAS_TRACE
#define def_EHelper(name) \
  s ++; \
  goto finish_label; /* this is for the previous def_EHelper() */ \
  def_label(concat(exec_, name))\
  trace_gather_info(s, #name);
#else
#define def_EHelper(name) \
  s ++; \
  goto finish_label; /* this is for the previous def_EHelper() */ \
  def_label(concat(exec_, name))
#endif
#define def_finish() def_label(finish_label)
#else
#define def_EHelper(name) static inline void concat(exec_, name) (Decode *s)
#endif

#if 0
#define IDEXW(idx, id, ex, w) CASE_ENTRY(idx, concat(decode_, id), concat(exec_, ex), w)
#define IDEX(idx, id, ex)     IDEXW(idx, id, ex, 0)
#define EXW(idx, ex, w)       IDEXW(idx, empty, ex, w)
#define EX(idx, ex)           EXW(idx, ex, 0)
#define EMPTY(idx)            EX(idx, inv)

// set_width() is defined in src/isa/$isa/exec/exec.c
#define CASE_ENTRY(idx, id, ex, w) case idx: set_width(s, w); id(s); ex(s); break;
#endif

#endif
