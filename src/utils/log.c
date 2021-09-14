#include <utils.h>

#ifndef CONFIG_TRACE_START
#define CONFIG_TRACE_START 0
#define CONFIG_TRACE_END   0
#endif

FILE *log_fp = NULL;

void init_log(const char *log_file) {
  if (log_file == NULL) return;
  log_fp = fopen(log_file, "w");
  Assert(log_fp, "Can not open '%s'", log_file);
}

bool log_enable() {
  extern uint64_t g_nr_guest_instr;
  return (g_nr_guest_instr >= CONFIG_TRACE_START) &&
         (g_nr_guest_instr <= CONFIG_TRACE_END);
}

char log_bytebuf[50] = {};
char log_asmbuf[128] = {};
