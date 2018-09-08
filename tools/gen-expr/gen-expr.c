#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <string.h>

// this should be enough
static char buf[65536];
static char *pbuf;

#define format_buf(fmt, ...) pbuf += sprintf(pbuf, fmt, ##__VA_ARGS__)

static inline uint32_t choose(uint32_t max) {
  return rand() % max;
}

static inline void gen_rand_op() {
  char op_list[4] = {'+', '-', '*', '/'};
  format_buf("%c", op_list[choose(4)]);
}

static inline void gen_num() {
  format_buf("%uu", choose(64));
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
  switch (choose(4)) {
    case 0:
      if (nr_op == 0) gen_rand_expr();
      else gen_num();
      break;
    case 1:
      format_buf("(");
      gen_rand_expr();
      format_buf(")");
      break;
    default:
      nr_op ++;
      if (choose(2)) gen_rand_expr();
      else gen_num();
      gen_space();
      gen_rand_op();
      gen_space();
      if (choose(2)) gen_rand_expr();
      else gen_num();
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

static char code_buf[65536];
static char *code_format =
"#include <stdio.h>\n"
"int main() { "
"  unsigned result = %s; "
"  printf(\"%%u\", result); "
"  return 0; "
"}";

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

    FILE *fp = fopen(".code.c", "w");
    assert(fp != NULL);
    fputs(code_buf, fp);
    fclose(fp);

    int ret = system("gcc .code.c -o .expr");
    if (ret != 0) continue;

    fp = popen("./.expr", "r");
    assert(fp != NULL);

    int result;
    fscanf(fp, "%d", &result);
    pclose(fp);
    remove_u(buf);
    printf("%u %s\n", result, buf);
  }
  return 0;
}
