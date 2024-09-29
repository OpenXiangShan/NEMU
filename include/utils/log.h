#ifndef __LOGS_H__
#define __LOGS_H__

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#ifdef CONFIG_CC_GPP
extern "C" {
#endif

extern uint64_t record_row_number;
extern FILE *log_fp;
extern char *log_filebuf;
extern int loop_index;
extern bool enable_fast_log;
extern bool enable_small_log;

void log_buffer_flush();
#ifdef CONFIG_CC_GPP
}
#endif

void init_log(const char *log_file, const bool fast_log, const bool small_log);
void log_file_flush(bool log_close);
void log_close();

#endif
