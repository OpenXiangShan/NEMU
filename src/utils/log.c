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

#include <utils.h>
#include <malloc.h>
// debug workload enable
//#define DEBUG_WORKLOAD_EN
// control when the log is printed, unit: number of instructions
#define DEBUG_POINT (0)
#define LOG_START ((DEBUG_POINT - (1024 * 10)) < 0 ? 0 : (DEBUG_POINT - (1024 * 10)))// 1024 X 10 instructions are logged
// restrict the size of log file
#define LOG_END    (DEBUG_POINT + 1024 * 10)
#define SMALL_LOG_ROW_NUM (100 * 1024) // row number
uint64_t record_row_number = 0;
FILE *log_fp = NULL;
char *log_filebuf;
bool enable_small_log = false;
extern bool workload_loaded;
void init_log(const char *log_file, const bool small_log) {
  if (log_file == NULL) return;
  log_fp = fopen(log_file, "w");
  Assert(log_fp, "Can not open '%s'", log_file);
  enable_small_log = small_log;
  if (enable_small_log)
    log_filebuf = (char *)malloc(sizeof(char) * SMALL_LOG_ROW_NUM * 300);
}

bool log_enable() {
  extern uint64_t g_nr_guest_instr;
#ifdef DEBUG_WORKLOAD_EN
  return workload_loaded && (g_nr_guest_instr > LOG_START) && (g_nr_guest_instr < LOG_END);
#else 
  return (g_nr_guest_instr >= LOG_START) && (g_nr_guest_instr <= LOG_END);
#endif
}

void log_flush() {
  record_row_number ++;
  if(record_row_number > SMALL_LOG_ROW_NUM){
    // rewind(log_fp);
    record_row_number = 0;
  }
}
void log_close(){
  if (enable_small_log == false) return;
  if (log_fp == NULL) return;
  // fprintf(log_fp, "%s", log_filebuf);
  for (int i = 0; i < record_row_number; i++)
  {
    fprintf(log_fp, "%s", log_filebuf + i * 300);
  }
  fclose(log_fp);
  free(log_filebuf);
}
char log_bytebuf[80] = {};
char log_asmbuf[80] = {};
