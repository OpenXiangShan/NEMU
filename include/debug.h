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
#include <utils/utils.h>
#include <unistd.h>
#include <utils/log.h>

// ----------- log -----------

/* #define log_write(...) MUXDEF(CONFIG_DEBUG, \
  do { \
    extern FILE* log_fp; \
    extern void log_flush(); \
    if (log_fp != NULL) { \
      log_flush(); \
      fprintf(log_fp, __VA_ARGS__); \
      fflush(log_fp); \
    }else{ \
      printf(__VA_ARGS__); \
    } \
  } while (0), \
  do { \
    printf(__VA_ARGS__); \
  }while (0)\
 )*/

// control when the log is printed, unit: number of instructions

#define LOG_START (0)
#define LOG_END   (1024 * 1024 * 50)

#define SMALL_LOG_ROW_NUM (50 * 1024 * 1024) // row number, 50M instructions
#define SMALL_LOG_ROW_BYTES 512

#define log_write(...) \
  do { \
    if (log_fp != NULL) { \
      if (enable_fast_log || enable_small_log) { \
        snprintf(log_filebuf + record_row_number * SMALL_LOG_ROW_BYTES, SMALL_LOG_ROW_BYTES, __VA_ARGS__);\
        log_buffer_flush(); \
      } else { \
        fprintf(log_fp, __VA_ARGS__); \
        fflush(log_fp); \
      } \
    }else{ \
      printf(__VA_ARGS__); \
    } \
  } while (0)

#define _Log(...) \
  do { \
    log_write(__VA_ARGS__); \
  } while (0)

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
    if (flag == dflag_simpoint && ISDEF(CONFIG_SIMPOINT_LOG)) Log(__VA_ARGS__); \
  } while (0)

#define Logm(...) Logf(dflag_mem, __VA_ARGS__)
#define Logtr(...) Logf(dflag_translate, __VA_ARGS__)
#define Logtb(...) Logf(dflag_trace_bb, __VA_ARGS__)
#define Logti(...) Logf(dflag_trace_inst, __VA_ARGS__)
#define Logtid(...) Logf(dflag_trace_inst_dasm, __VA_ARGS__)
#define Loge(...) Logf(dflag_exit, __VA_ARGS__)
#define Logsp(...) Logf(dflag_simpoint, __VA_ARGS__)

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
