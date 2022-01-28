#include <common.h>

extern uint64_t g_nr_guest_instr;
FILE *log_fp = NULL;
FILE *bb_fp = NULL;
FILE *inst_fp = NULL;
extern char* img_file;
void init_log(const char *log_file) {
  log_fp = stdout;
  if (log_file != NULL) {
    FILE *fp = fopen(log_file, "w");
    Assert(fp, "Can not open '%s'", log_file);
    log_fp = fp;
  }
#ifndef CONFIG_MODE_USER
  Log("Log is written to %s", log_file ? log_file : "stdout");
#endif

#if defined(CONFIG_BB_COUNT) || defined(CONFIG_EHELPER_COUNT)
  char file_name[128];
  char bb_log_name[128];
  memset(file_name, 0, sizeof(file_name));
  strcpy(file_name, img_file);
  char* surfix = strtok(file_name, "/");
  char* p;
  while((p = strtok(NULL, "/"))){
    surfix = p;
  }
#ifdef CONFIG_BB_COUNT
  memset(bb_log_name, 0, sizeof(bb_log_name));
  strcpy(bb_log_name, "/home/chenlu/program/nemu/build/bb-");
  strcpy(bb_log_name + strlen(bb_log_name), surfix);
  Assert(bb_log_name, "img_file: %s\n", img_file);
  bb_fp = fopen(bb_log_name, "w");
#endif
#ifdef CONFIG_EHELPER_COUNT
  memset(bb_log_name, 0, sizeof(bb_log_name));
  strcpy(bb_log_name, "/home/chenlu/program/nemu/build/inst-");
  strcpy(bb_log_name + strlen(bb_log_name), surfix);
  Assert(bb_log_name, "img_file: %s\n", img_file);
  inst_fp = fopen(bb_log_name, "w");
#endif
#endif
}

bool log_enable() {
  return MUXDEF(CONFIG_TRACE, (g_nr_guest_instr >= CONFIG_TRACE_START) &&
         (g_nr_guest_instr <= CONFIG_TRACE_END), false);
}
