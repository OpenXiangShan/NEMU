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

#include <trace/spikedasm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// We cache the spike-dasm state here. If it is changed during running, the behavior is undefined.
static bool is_tested = false, is_valid = true;

// This is a place-holder command string. spike_dasm function will replace the inst.
static char dasm_cmd[] = "echo \"DASM(01234567deadbeaf)\" | spike-dasm";
static const int dasm_offset = sizeof("echo \"DASM(") - 1;

// We are using a global string as the result buffer.
static char dasm_result[32];

bool spike_valid() {
  if (!is_tested) {
    is_tested = true;
    char cmd[128];
    sprintf(cmd, "%s > /dev/null 2> /dev/null", dasm_cmd);
    is_valid = !system(cmd);
  }
  return is_valid;
}

static void execute_dasm_cmd() {
  FILE *ptr = popen(dasm_cmd, "r");
  if (ptr) {
    char *res = fgets(dasm_result, sizeof(dasm_result), ptr);
    if (res == NULL) {
      printf("spike dasm fgets error\n");
    }
    pclose(ptr);
  } else {
    printf("popen %s error\n", dasm_cmd);
  }
}

const char *spike_dasm(uint64_t inst) {
  char inst_hex_string[17];
  sprintf(inst_hex_string, "%016lx", inst);
  memcpy(dasm_cmd + dasm_offset, inst_hex_string, 16);
  execute_dasm_cmd();
  char *first_n_occ = strpbrk(dasm_result, "\n");
  if (first_n_occ)
    *first_n_occ = '\0';
  return dasm_result;
}

/*
This is an example usage. Not used.

static int usage() {
  uint64_t input = 0x10500073L;

  if (spike_valid()) {
    printf("%s", spike_dasm(input));
  }

  return 0;
}
*/
