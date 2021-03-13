#include <utils.h>

// control when the log is printed, unit: number of instructions
#define LOG_START (0)
// restrict the size of log file
#define LOG_END   (1024 * 1024 * 50)

FILE *log_fp = NULL;

void init_log(const char *log_file) {
  if (log_file == NULL) return;
  log_fp = fopen(log_file, "w");
  Assert(log_fp, "Can not open '%s'", log_file);
}

bool log_enable() {
  extern uint64_t g_nr_guest_instr;
  return (g_nr_guest_instr >= LOG_START) && (g_nr_guest_instr <= LOG_END);
}

char log_bytebuf[80] = {};
char log_asmbuf[80] = {};
