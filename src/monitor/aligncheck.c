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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <common.h>

#ifdef CONFIG_AC_HOST
#ifndef CONFIG_SHARE
#include <signal.h>

#define RFLAGS_AC (1u << 18)
#define RFLAGS_TF (1u << 8)

static void aligncheck_sig_handler(int sig, siginfo_t *info, void *ucontext) {
  ucontext_t *uc = (ucontext_t *)ucontext;
  uint8_t *rip = (uint8_t *)uc->uc_mcontext.gregs[REG_RIP];
  extern uint8_t _start, _etext;
  if (&_start <= rip && rip < &_etext) {
    panic("handle misaligned memory accessing");
  }
  uc->uc_mcontext.gregs[REG_EFL] &= ~RFLAGS_AC;
  uc->uc_mcontext.gregs[REG_EFL] |=  RFLAGS_TF;
}

static void trap_sig_handler(int sig, siginfo_t *info, void *ucontext) {
  ucontext_t *uc = (ucontext_t *)ucontext;
  uc->uc_mcontext.gregs[REG_EFL] |=  RFLAGS_AC;
  uc->uc_mcontext.gregs[REG_EFL] &= ~RFLAGS_TF;
}

static inline void aligncheck_enable() {
  asm volatile(
      "add $-128, %%rsp \n"    // skip past the red-zone
      "pushf\n"
      "orl $0x40000,(%%rsp)\n"
      "popf \n"
      "sub $-128, %%rsp"       // and restore the stack pointer.
      ::: "memory");       // ordered wrt. other mem access
}

void init_aligncheck() {
  struct sigaction s;
  memset(&s, 0, sizeof(s));
  s.sa_sigaction = aligncheck_sig_handler;
  s.sa_flags = SA_SIGINFO;
  int ret = sigaction(SIGBUS, &s, NULL);
  Assert(ret == 0, "Can not set signal handler");

  memset(&s, 0, sizeof(s));
  s.sa_sigaction = trap_sig_handler;
  s.sa_flags = SA_SIGINFO;
  ret = sigaction(SIGTRAP, &s, NULL);
  Assert(ret == 0, "Can not set signal handler");

  aligncheck_enable();
}
#else
void init_aligncheck() {
}
#endif
#else
void init_aligncheck() {
}
#endif
