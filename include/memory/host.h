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

#ifndef __MEMORY_HOST_H__
#define __MEMORY_HOST_H__

#include <common.h>

static inline word_t host_read(void *addr, int len) {
  switch (len) {
    case 1:
    Logm("load: addr = %p, len = %d, data = 0x%x", addr, len, *(uint8_t  *)addr);
    return *(uint8_t  *)addr;
    case 2:
    Logm("load: addr = %p, len = %d, data = 0x%x", addr, len, *(uint16_t  *)addr);
    return *(uint16_t *)addr;
    case 4:
    Logm("load: addr = %p, len = %d, data = 0x%x", addr, len, *(uint32_t  *)addr);
    return *(uint32_t *)addr;
#ifdef CONFIG_ISA64
    case 8:
    Logm("load: addr = %p, len = %d, data = 0x%lx", addr, len, *(uint64_t  *)addr);
    return *(uint64_t *)addr;
#endif
    default: MUXDEF(CONFIG_RT_CHECK, assert(0), return 0);
  }
}

static inline void host_write(void *addr, int len, word_t data) {
  Logm("write: addr = %p, len = %d, data = 0x%lx", addr, len, data);
  switch (len) {
    case 1: *(uint8_t  *)addr = data; return;
    case 2: *(uint16_t *)addr = data; return;
    case 4: *(uint32_t *)addr = data; return;
    IFDEF(CONFIG_ISA64, case 8: *(uint64_t *)addr = data; return);
    IFDEF(CONFIG_RT_CHECK, default: assert(0));
  }
}

#endif
