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

#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <stdio.h>
#include <assert.h>
#include <utils.h>
#include <unistd.h>

#define Log(format, ...) \
    _Log("\33[1;34m[%s:%d,%s] " format "\33[0m\n", \
        __FILE__, __LINE__, __func__, ## __VA_ARGS__)

#define Logf(flag, ...) \
  do { \
    extern bool log_enable();\
    if (ISDEF(CONFIG_MEMLOG) && flag == dflag_mem && log_enable()) Log(__VA_ARGS__); \
    if (ISDEF(CONFIG_TRANSLOG) && flag == dflag_translate && log_enable()) Log(__VA_ARGS__); \
    if (ISDEF(CONFIG_TRACE_INST) && flag == dflag_trace_inst && log_enable()) Log(__VA_ARGS__); \
    if (ISDEF(CONFIG_TRACE_INST_DASM) && flag == dflag_trace_inst_dasm && log_enable()) Log(__VA_ARGS__); \
    if (ISDEF(CONFIG_TRACE_CALL_RET) && flag == dflag_trace_call_ret && log_enable()) Log(__VA_ARGS__); \
    if (ISDEF(CONFIG_TRACE_BB) && flag == dflag_trace_bb && log_enable()) Log(__VA_ARGS__); \
    if (ISDEF(CONFIG_EXITLOG) && flag == dflag_exit) Log(__VA_ARGS__); \
    if (ISDEF(CONFIG_SIMPOINT_LOG) && flag == dflag_simpoint) Log(__VA_ARGS__); \
  } while (0)

#define Logm(...) IFDEF(CONFIG_MEMLOG,Logf(dflag_mem, __VA_ARGS__))
#define Logtr(...) IFDEF(CONFIG_TRANSLOG,Logf(dflag_translate, __VA_ARGS__))
#define Logtb(...) IFDEF(CONFIG_TRACE_BB,Logf(dflag_trace_bb, __VA_ARGS__))
#define Logti(...) IFDEF(CONFIG_TRACE_INST,Logf(dflag_trace_inst, __VA_ARGS__))
#define Logtid(...) IFDEF(CONFIG_TRACE_INST_DASM,Logf(dflag_trace_inst_dasm, __VA_ARGS__))
#define Loge(...) IFDEF(CONFIG_EXITLOG,Logf(dflag_exit, __VA_ARGS__))
#define Logc(...) IFDEF(CONFIG_TRACE_CSR,Logf(dflag_trace_call_ret, __VA_ARGS__))
#define Logsp(...) IFDEF(CONFIG_SIMPOINT_LOG,Logf(dflag_simpoint, __VA_ARGS__))

#define Assert(cond, ...) \
  do { \
    if (!(cond)) { \
      fflush(stdout); \
      fprintf(stderr, "\33[1;31m"); \
      fprintf(stderr, __VA_ARGS__); \
      fprintf(stderr, "\33[0m\n"); \
      extern void isa_reg_display(); \
      extern void monitor_statistic(); \
      isa_reg_display(); \
      monitor_statistic(); \
      assert(cond); \
    } \
  } while (0)

#define panic(...) Assert(0, __VA_ARGS__)

#define fprintf_with_pid(stream, ...) \
  do { \
    fprintf(stream, "(%d) ", getpid()); \
    fprintf(stream, __VA_ARGS__); \
  }while(0)

#define printf_with_pid(...) \
  do { \
    fprintf_with_pid(stdout, __VA_ARGS__); \
  }while(0)

#define xpanic(...) \
  do { \
      printf("\33[1;31m"); \
      printf(__VA_ARGS__); \
      printf("\33[0m\n"); \
      assert(0); \
  } while (0)

#define TODO() panic("please implement me")

#endif
