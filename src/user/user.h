#ifndef __USER_H__
#define __USER_H__

typedef struct {
  word_t brk;
  word_t program_brk;
} user_state_t;

extern user_state_t user_state;

#endif
