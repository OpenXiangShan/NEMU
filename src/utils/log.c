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

uint64_t record_row_number = 0;
FILE *log_fp = NULL;
char *log_filebuf = NULL;
int loop_index = 0;
bool enable_fast_log = false;
bool enable_small_log = false;

void init_log(const char *log_file, const bool fast_log, const bool small_log) {
  if (log_file == NULL) return;
  log_fp = fopen(log_file, "w");
  Assert(log_fp, "Can not open '%s'", log_file);

  if(fast_log || small_log){
    log_filebuf = (char*)malloc(sizeof(char) * SMALL_LOG_ROW_NUM * SMALL_LOG_ROW_BYTES);
    Assert(log_filebuf, "Can not alloc memory for log_filebuf");
    memset(log_filebuf, 0, sizeof(char) * SMALL_LOG_ROW_NUM * SMALL_LOG_ROW_BYTES);
  }

  enable_fast_log = fast_log;
  enable_small_log = small_log;
}

void log_file_flush(bool log_close) {
  if (!enable_small_log) {
    for (uint64_t i = 0;
         i < (SMALL_LOG_ROW_NUM < record_row_number ? SMALL_LOG_ROW_NUM
                                                    : record_row_number);
         i++) {
      fprintf(log_fp, "%s", log_filebuf + i * SMALL_LOG_ROW_BYTES);
    }
    fflush(log_fp);
  } else if (log_close) {
    int loop_start_index = record_row_number;
    for (int i = loop_start_index; i < SMALL_LOG_ROW_NUM; i++) {
      if (*(log_filebuf + i * SMALL_LOG_ROW_BYTES) != 0) {
        fprintf(log_fp, "%s", log_filebuf + i * SMALL_LOG_ROW_BYTES);
      }
    }
    for (uint64_t i = 0; i < record_row_number; i++) {
      fprintf(log_fp, "%s", log_filebuf + i * SMALL_LOG_ROW_BYTES);
    }
  }
}

void log_buffer_flush() {
  record_row_number++;
  if (record_row_number > SMALL_LOG_ROW_NUM) {
    log_file_flush(false);
    record_row_number = 0;
  }
}

void log_close() {
  if (!log_fp && !log_filebuf) {
    return;
  }

  if (log_filebuf) {
    log_file_flush(true);
    free(log_filebuf);
  }

  if (log_fp) {
    fclose(log_fp);
  }
}
