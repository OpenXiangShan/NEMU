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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <string.h>

// this should be enough
static char buf[65536] = {};
static char code_buf[65536 + 128] = {}; // a little larger than `buf`
static char *code_format =
"#include <stdio.h>\n"
"int main() { "
"  unsigned result = %s; "
"  printf(\"%%u\", result); "
"  return 0; "
"}";

static char *pbuf;

#define format_buf(fmt, ...) pbuf += sprintf(pbuf, fmt, ##__VA_ARGS__)

static inline uint32_t choose(uint32_t max) {
  return rand() % max;
}

static inline void gen_rand_op() {
  char op_list[] = {'+', '-', '*', '/', '+', '-', '*'};
  format_buf("%c", op_list[choose(7)]);
}

static inline void gen_num() {
  format_buf("%uu", rand());
}

static inline void gen_space() {
  char *space_list[3] = {
    "",
    " ",
    "  ",
  };
  format_buf("%s", space_list[choose(3)]);
}

static int nr_op = 0;

static inline void gen_rand_expr() {
  gen_space();
  switch (choose(3)) {
    default:
      if (nr_op == 0) gen_rand_expr();
      else gen_num();
      break;
    case 1:
      format_buf("(");
      gen_rand_expr();
      format_buf(")");
      break;
    case 0:
      nr_op ++;
      if (pbuf - buf >= sizeof(buf) / 2) {
        gen_num();
        break;
      }
      gen_rand_expr();
      gen_space();
      gen_rand_op();
      gen_space();
      gen_rand_expr();
      break;
  }
  gen_space();
}

void remove_u(char *p) {
  char *q = p;
  while ((q = strchr(q, 'u')) != NULL) {
    // reuse code_buf
    strcpy(code_buf, q + 1);
    strcpy(q, code_buf);
  }
}

int main(int argc, char *argv[]) {
  int seed = time(0);
  srand(seed);
  int loop = 1;
  if (argc > 1) {
    sscanf(argv[1], "%d", &loop);
  }
  int i;
  for (i = 0; i < loop; i ++) {
    nr_op = 0;
    pbuf = buf;
    gen_rand_expr();

    sprintf(code_buf, code_format, buf);

    FILE *fp = fopen("/tmp/.code.c", "w");
    assert(fp != NULL);
    fputs(code_buf, fp);
    fclose(fp);

    int ret = system("gcc /tmp/.code.c -o /tmp/.expr");
    if (ret != 0) continue;

    fp = popen("/tmp/.expr", "r");
    assert(fp != NULL);

    int result;
    fscanf(fp, "%d", &result);
    ret = pclose(fp);
    if (ret != 0) continue;

    remove_u(buf);

    printf("%u %s\n", result, buf);
  }
  return 0;
}
