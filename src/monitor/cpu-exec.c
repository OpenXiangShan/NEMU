#include <isa.h>
#include <monitor/monitor.h>
#include <monitor/difftest.h>
#include <stdlib.h>
#include <time.h>
#include <locale.h>
#ifndef __ICS_EXPORT
#include "debug/watchpoint.h"
#endif

/* The assembly code of instructions executed is only output to the screen
 * when the number of instructions executed is less than this value.
 * This is useful when you use the `si' command.
 * You can modify this value as you want.
 */
#define MAX_INSTR_TO_PRINT 10

// control when the log is printed, unit: number of instructions
#define LOG_START (0)
// restrict the size of log file
#define LOG_END   (1024 * 1024 * 50)

CPU_state cpu = {};
NEMUState nemu_state = { .state = NEMU_STOP };
uint64_t g_nr_guest_instr = 0;
static uint64_t g_timer = 0; // unit: us
static bool g_print_step = false;
const rtlreg_t rzero = 0;

void asm_print(vaddr_t this_pc, int instr_len, bool print_flag);

int is_exit_status_bad() {
  int good = (nemu_state.state == NEMU_END && nemu_state.halt_ret == 0) ||
    (nemu_state.state == NEMU_QUIT);
  return !good;
}

void monitor_statistic() {
  setlocale(LC_NUMERIC, "");
  Log("total guest instructions = %'ld", g_nr_guest_instr);
  Log("host time spent = %'ld us", g_timer);
  if (g_timer > 0) Log("simulation frequency = %'ld instr/s", g_nr_guest_instr * 1000000 / g_timer);
  else Log("Finish running in less than 1 us and can not calculate the simulation frequency");
}

bool log_enable() {
  return (g_nr_guest_instr >= LOG_START) && (g_nr_guest_instr <= LOG_END);
}

uint64_t get_time() {
  struct timespec now;
  clock_gettime(CLOCK_MONOTONIC_COARSE, &now);
  uint64_t us = now.tv_sec * 1000000 + now.tv_nsec / 1000;
  return us;
}

#ifdef DEBUG
void debug_hook(vaddr_t this_pc, int len) {
  asm_print(this_pc, len, g_print_step);

  /* TODO: check watchpoints here. */
#ifndef __ICS_EXPORT
  WP *wp = scan_watchpoint();
  if(wp != NULL) {
    printf("\n\nHint watchpoint %d at address " FMT_WORD ", expr = %s\n", wp->NO, this_pc, wp->expr);
    printf("old value = " FMT_WORD "\nnew value = " FMT_WORD "\n", wp->old_val, wp->new_val);
    wp->old_val = wp->new_val;
    nemu_state.state = NEMU_STOP;
  }
#endif
}
#endif

void execute(uint64_t n);

/* Simulate how the CPU works. */
void cpu_exec(uint64_t n) {
  g_print_step = (n < MAX_INSTR_TO_PRINT);
  switch (nemu_state.state) {
    case NEMU_END: case NEMU_ABORT:
      printf("Program execution has ended. To restart the program, exit NEMU and run again.\n");
      return;
    default: nemu_state.state = NEMU_RUNNING;
  }

  uint64_t timer_start = get_time();

  while (nemu_state.state == NEMU_RUNNING) {
    execute(n);
#ifdef HAS_IOE
    extern void device_update();
    device_update();
#endif

#if !defined(DIFF_TEST) && !_SHARE
    void query_intr();
    query_intr();
#endif
  }

  uint64_t timer_end = get_time();
  g_timer += timer_end - timer_start;

  switch (nemu_state.state) {
    case NEMU_RUNNING: nemu_state.state = NEMU_STOP; break;

    case NEMU_END: case NEMU_ABORT:
      Log("nemu: %s\33[0m at pc = " FMT_WORD "\n\n",
          (nemu_state.state == NEMU_ABORT ? "\33[1;31mABORT" :
           (nemu_state.halt_ret == 0 ? "\33[1;32mHIT GOOD TRAP" : "\33[1;31mHIT BAD TRAP")),
          nemu_state.halt_pc);
      // fall through
    case NEMU_QUIT:
      monitor_statistic();
  }
}
