#ifndef __UTILS_H__
#define __UTILS_H__

#include <common.h>

// ----------- state -----------

enum { NEMU_RUNNING, NEMU_STOP, NEMU_END, NEMU_ABORT, NEMU_QUIT };

typedef struct {
  int state;
  vaddr_t halt_pc;
  uint32_t halt_ret;
} NEMUState;

extern NEMUState nemu_state;

enum {
  dflag_none = 0,
  dflag_mem,
  dflag_translate,
  dflag_trace_bb,
  dflag_trace_inst,
  dflag_trace_inst_dasm,
  dflag_exit,
};


// ----------- timer -----------

uint64_t get_time();

// ----------- log -----------

#define log_write(...) IFDEF(CONFIG_DEBUG, \
  do { \
    extern FILE* log_fp; \
    extern bool log_enable(); \
    if (log_fp != NULL && log_enable()) { \
      fprintf(log_fp, __VA_ARGS__); \
      fflush(log_fp); \
    } \
  } while (0) \
)

#define _Log(...) \
  do { \
    printf(__VA_ARGS__); \
    log_write(__VA_ARGS__); \
  } while (0)

extern char log_bytebuf[80];
extern char log_asmbuf[80];

// ----------- expr -----------

word_t expr(char *e, bool *success);

// ----------- iqueue -----------
void iqueue_commit(vaddr_t pc, uint8_t *instr_buf, uint8_t ilen);
void iqueue_dump();

#ifdef __cplusplus
extern "C" {
#endif
bool is_gz_file(const char *filename);
#ifdef __cplusplus
}
#endif

#endif
