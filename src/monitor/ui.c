#include <isa.h>
#include <utils.h>
#include <cpu/cpu.h>
#ifndef __ICS_EXPORT
#include <memory/paddr.h>
#include <memory/vaddr.h>
#include <cpu/difftest.h>
#endif

#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

#ifndef CONFIG_SHARE
int is_batch_mode();
int set_watchpoint(char *e);
bool delete_watchpoint(int NO);
void list_watchpoint();

/* We use the `readline' library to provide more flexibility to read from stdin. */
static char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec((args == NULL) ? -1 : atoi(args));
  return 0;
}

#ifndef __ICS_EXPORT
static int cmd_si(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");

  if (arg == NULL) {
    /* no argument given */
    cpu_exec(1);
  }
  else {
    int n = strtol(arg, NULL, 10);
    printf("si %d\n", n);
    cpu_exec(n);
  }
  return 0;
}

static int cmd_info(char *args) {
  char *arg = strtok(NULL, " ");

  if (arg == NULL) {
    /* no argument given */
    Log("usage: info [r|w]");
  }
  else {
    if (strcmp(arg, "r") == 0) {
      isa_reg_display();
    }
    else if (strcmp(arg, "w") == 0) {
      list_watchpoint();
    }
  }
  return 0;
}

static int cmd_d(char *args) {
  char *arg = strtok(NULL, " ");

  if (arg == NULL) {
    /* no argument given */
    Log("usage: d n");
  }
  else {
    int NO;
    sscanf(args, "%d", &NO);
    if (!delete_watchpoint(NO)) {
      printf("Watchpoint #%d does not exist\n", NO);
    }
  }

  return 0;
}

static int cmd_p(char *args) {
  if (args != NULL) {
    bool success;
    word_t r = expr(args, &success);
    if(success) { printf(FMT_WORD "\n", r); }
    else { printf("Bad expression\n"); }
  }
  return 0;
}

static int cmd_w(char *args) {
  if (args != NULL) {
    int NO = set_watchpoint(args);
    if (NO != -1) { printf("Set watchpoint #%d\n", NO); }
    else { printf("Bad expression\n"); }
  }
  return 0;
}

static int cmd_x(char *args) {
  char *arg = strtok(NULL, " ");

  if (arg == NULL) {
    /* no argument given */
    Log("usage: x n addr");
  }
  else {
    int n;
    vaddr_t addr;
    int i;
    sscanf(arg, "%d", &n);

    bool success;
    addr = expr(arg + strlen(arg) + 1, &success);
    if (success) {
      for (i = 0; i < n; i ++) {
        if (i % 4 == 0) {
          printf(FMT_WORD ": ", addr);
        }

        printf("0x%08x ", (uint32_t)vaddr_read_safe(addr, 4));
        addr += 4;
        if (i % 4 == 3) {
          printf("\n");
        }
      }
      printf("\n");
    }
    else { printf("Bad expression\n"); }
  }
  return 0;
}

#ifdef CONFIG_MODE_SYSTEM
static int cmd_detach(char *args) {
  difftest_detach();
  return 0;
}

static int cmd_attach(char *args) {
  difftest_attach();
  return 0;
}

static int cmd_save(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");

  if (arg == NULL) {
    /* no argument given */
    Log("no path");
  }
  else {
    FILE *fp = fopen(arg, "w");
    assert(fp != NULL);
    fwrite(&cpu, sizeof(cpu), 1, fp);
    fwrite(guest_to_host(CONFIG_MBASE), CONFIG_MSIZE, 1, fp);
    fclose(fp);
  }
  return 0;
}

static int cmd_load(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");

  if (arg == NULL) {
    /* no argument given */
    Log("no path");
  }
  else {
    FILE *fp = fopen(arg, "r");
    assert(fp != NULL);
    __attribute__((unused)) int ret;
    ret = fread(&cpu, sizeof(cpu), 1, fp);
    ret = fread(guest_to_host(CONFIG_MBASE), CONFIG_MSIZE, 1, fp);
    fclose(fp);
  }
  return 0;
}
#else
#endif

#endif

static int cmd_q(char *args) {
  return -1;
}

static int cmd_help(char *args);

static struct {
  const char *name;
  const char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display informations about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
#ifndef __ICS_EXPORT
  { "si", "step", cmd_si },
  { "info", "info r - print register values; info w - show watch point state", cmd_info },
  { "x", "Examine memory", cmd_x },
  { "p", "Evaluate the value of expression", cmd_p },
  { "w", "Set watchpoint", cmd_w },
  { "d", "Delete watchpoint", cmd_d },
#ifdef CONFIG_MODE_SYSTEM
  { "detach", "detach diff test", cmd_detach },
  { "attach", "attach diff test", cmd_attach },
  { "save", "save snapshot", cmd_save },
  { "load", "load snapshot", cmd_load },
#endif
#endif
  { "q", "Exit NEMU", cmd_q },

  /* TODO: Add more commands */

};

#define NR_CMD ARRLEN(cmd_table)

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

void ui_mainloop() {
  if (is_batch_mode()) {
    extern char *max_instr;
    cmd_c(max_instr);
    return;
  }

  for (char *str; (str = rl_gets()) != NULL; ) {
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef CONFIG_DEVICE
    extern void sdl_clear_event_queue();
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}
#endif
