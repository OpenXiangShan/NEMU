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
    if (flag == dflag_mem && ISDEF(CONFIG_MEMLOG)) Log(__VA_ARGS__); \
    if (flag == dflag_translate && ISDEF(CONFIG_TRANSLOG)) Log(__VA_ARGS__); \
    if (flag == dflag_trace_inst && ISDEF(CONFIG_TRACE_INST)) Log(__VA_ARGS__); \
    if (flag == dflag_trace_inst_dasm && ISDEF(CONFIG_TRACE_INST_DASM)) Log(__VA_ARGS__); \
    if (flag == dflag_trace_bb && ISDEF(CONFIG_TRACE_BB)) Log(__VA_ARGS__); \
    if (flag == dflag_exit && ISDEF(CONFIG_EXITLOG)) Log(__VA_ARGS__); \
  } while (0)

#define Logm(...) Logf(dflag_mem, __VA_ARGS__)
#define Logtr(...) Logf(dflag_translate, __VA_ARGS__)
#define Logtb(...) Logf(dflag_trace_bb, __VA_ARGS__)
#define Logti(...) Logf(dflag_trace_inst, __VA_ARGS__)
#define Logtid(...) Logf(dflag_trace_inst_dasm, __VA_ARGS__)
#define Loge(...) Logf(dflag_exit, __VA_ARGS__)

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
