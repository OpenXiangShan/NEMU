/***************************************************************************************
* Copyright (c) 2020-2023 Institute of Computing Technology, Chinese Academy of Sciences
* Copyright (c) 2020-2021 Peng Cheng Laboratory
*
* DiffTest is licensed under Mulan PSL v2.
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

#ifndef __TRACE_COMMON_H__
#define __TRACE_COMMON_H__

// Control
#define TRACE_DUMP_PAGE_TABLE
#define TRACE_PAGE_SIZE (4096)
#define TRACE_PAGE_SHIFT (12)
#define TRACE_PAGE_ENTRY_SIZE (sizeof(uint64_t))
#define TRACE_PAGE_ENTRY_NUM (4096 / TRACE_PAGE_ENTRY_SIZE)
#define TRACE_SATP64_PPN  (0x00000FFFFFFFFFFF)
#define TraceVPNi(vpn, i)        (((vpn) >> (9 * (i))) & 0x1ff)
#define GetPteAddr(vpn, level, baseAddr) \
  (baseAddr + TraceVPNi(vpn, level) * sizeof(uint64_t))



#if 1
#define trace_likely(cond)   __builtin_expect(cond, 1)
#define trace_unlikely(cond) __builtin_expect(cond, 0)
#else
#define trace_likely(cond)   (cond)
#define trace_unlikely(cond) (cond)
#endif

// #define Log() printf("file: %s, line: %d\n", __FILE__, __LINE__); fflush(stdout)

// #define VERBOSE
// #define LogBuffer
#define MORECHECK

#ifdef LogBuffer

#define TraceLogEntryNum 16
#define TraceLogEntrySize 256
char trace_log_buf[TraceLogEntryNum][TraceLogEntrySize];
uint8_t trace_log_ptr = 0;

uint8_t trace_log_ptr_pop() {
  return (trace_log_ptr ++) % TraceLogEntrySize;
}
// uint8_t trace_log_ptr_get() {
//   return trace_log_ptr % TraceLogEntrySize;
// }
#endif

#ifdef LogBuffer
#define WHEREAMI snprintf(trace_log_buf[trace_log_ptr_pop()], TraceLogEntrySize, "FromBuf:%s:%d\n", __FILE__, __LINE__);
#else
#define WHEREAMI printf("%s:%d:%s\n", __FILE__, __LINE__, __func__); fflush(stdout);
#endif

#define WHEREAMIDirect printf("Direct:%s:%d:%s\n", __FILE__, __LINE__, __func__); fflush(stdout);

#ifdef VERBOSE
#define TraceSimpleLog WHEREAMI

#ifdef LogBuffer
#define TraceLog(...) snprintf(trace_log_buf[trace_log_ptr_pop()], TraceLogEntrySize, __VA_ARGS__);
#else
#define TraceLog(...) printf(__VA_ARGS__); fflush(stdout);
#endif

#else
#define TraceSimpleLog
#define TraceLog(...)
#endif

#ifdef MORECHECK
#define TraceRequireInvalid                     \
  do {                                       \
    if (inst_valid) {                        \
      error_dump();                          \
      WHEREAMIDirect;                        \
      exit(1);                               \
    }                                        \
  } while(0);

#define TraceRequireValid                   \
  do {                                       \
    if (!inst_valid) {                       \
      error_dump();                          \
      WHEREAMIDirect;                        \
      exit(1);                               \
    }                                        \
  } while(0);
#else
#define TraceRequireInvalid
#define TraceRequireValid
#endif


#endif // __TRACE_COMMON_H__