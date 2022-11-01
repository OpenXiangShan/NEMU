#ifdef CONFIG_RVSDTRIG

#ifndef __TRIGGER_H__
#define __TRIGGER_H__

#include "common.h"
#include "csr.h"

typedef enum {
  TRIG_TYPE_NONE      = 0,
  TRIG_TYPE_LEGACY    = 1,  // legacy SiFive address match trigger
  TRIG_TYPE_MCONTROL  = 2,  // address/data match trigger
  TRIG_TYPE_ICOUNT    = 3,  // instruction count trigger
  TRIG_TYPE_ITRIG     = 4,  // interrupt trigger
  TRIG_TYPE_ETRIG     = 5,  // exception trigger
  TRIG_TYPE_MCONTROL6 = 6,  // address/data match trigger with additional functionality(better than mcontrol)
  TRIG_TYPE_TMEXTTRIG = 7,  // external trigger
  TRIG_TYPE_DISABLE   = 15, // disabled trigger
} trig_type_t;

typedef enum {
  TRIG_OP_EXECUTE = 0x1,
  TRIG_OP_STORE   = 0x2,
  TRIG_OP_LOAD    = 0x4,
  TRIG_OP_TIMING  = 0x10, // set if timing = 1
} trig_op_t;

typedef enum {
  TRIG_ACTION_BKPT_EXCPT    = 0, // raise a breakpoint exception, handled as exception
  TRIG_ACTION_DEBUG_MODE    = 1, // enter debug mode
  TRIG_ACTION_TRACE_ON      = 2,
  TRIG_ACTION_TRACE_OFF     = 3,
  TRIG_ACTION_TRACE_NOTIFY  = 4,
  TRIG_ACTION_NONE          = -1,
} trig_action_t;

typedef enum {
  TRIG_MATCH_EQ       = 0,
  TRIG_MATCH_NAPOT    = 1,
  TRIG_MATCH_GE       = 2,
  TRIG_MATCH_LT       = 3,
  TRIG_MATCH_MASK_LO  = 4,
  TRIG_MATCH_MASK_HI  = 5,
  TRIG_MATCH_NE       = 8,
  TRIG_MATCH_NNAPOT   = 9,  // not napot
  TRIG_MATCH_NMASK_LO = 12, // not mask low
  TRIG_MATCH_NMASK_HI = 13, // not mask high
} trig_match_t;

typedef struct {
  uint64_t load   : 1;  // [0]
  uint64_t store  : 1;  // [1]
  uint64_t execute: 1;  // [2]
  uint64_t u      : 1;  // [3]
  uint64_t s      : 1;  // [4]
  uint64_t pad0   : 1;  // [5]
  uint64_t m      : 1;  // [6]
  uint64_t match  : 4;  // [10:7]
  uint64_t chain  : 1;  // [11]
  uint64_t action : 4;  // [15:12]
  uint64_t sizelo : 2;  // [17:16]
  uint64_t timing : 1;  // [18]
  uint64_t select : 1;  // [19]
  uint64_t hit    : 1;  // [20]
  uint64_t sizehi : 2;  // [22:21]
  uint64_t pad1   : 30; // [52:23]
  uint64_t maskmax: 6;  // [58:53]
  uint64_t dmode  : 1;  // [59] 
  uint64_t type   : 4;  // [63:60]
} trig_mcontrol_t;

typedef struct {
  uint64_t load   : 1;  // [0]
  uint64_t store  : 1;  // [1]
  uint64_t execute: 1;  // [2]
  uint64_t u      : 1;  // [3]
  uint64_t s      : 1;  // [4]
  uint64_t pad0   : 1;  // [5]
  uint64_t m      : 1;  // [6]
  uint64_t match  : 4;  // [10:7]
  uint64_t chain  : 1;  // [11]
  uint64_t action : 4;  // [15:12]
  uint64_t size   : 4;  // [19:16]
  uint64_t timing : 1;  // [20]
  uint64_t select : 1;  // [21]
  uint64_t hit    : 1;  // [22]
  uint64_t vu     : 1;  // [23]
  uint64_t vs     : 1;  // [24]
  uint64_t pad1   : 34; // [58:25]
  uint64_t dmode  : 1;  // [59] 
  uint64_t type   : 4;  // [63:60]
} trig_mcontrol6_t;

typedef struct {
  uint64_t action : 6;  // [5:0]
  uint64_t u      : 1;  // [6]
  uint64_t s      : 1;  // [7]
  uint64_t pending: 1;  // [8]
  uint64_t m      : 1;  // [9]
  uint64_t count  : 14; // [23:10]
  uint64_t hit    : 1;  // [24]
  uint64_t vu     : 1;  // [25]
  uint64_t vs     : 1;  // [26]
  uint64_t pad    : 32; // [58:27]
  uint64_t dmode  : 1;  // [59] 
  uint64_t type   : 4;  // [63:60]
} trig_icount_t;

typedef struct {
  uint64_t action : 6;  // [5:0]
  uint64_t u      : 1;  // [6]
  uint64_t s      : 1;  // [7]
  uint64_t pad0   : 1;  // [8]
  uint64_t m      : 1;  // [9]
  uint64_t nmi    : 1;  // [10]
  uint64_t vu     : 1;  // [11]
  uint64_t vs     : 1;  // [12]
  uint64_t pad1   : 45; // [57:13]
  uint64_t hit    : 1;  // [58]
  uint64_t dmode  : 1;  // [59] 
  uint64_t type   : 4;  // [63:60]
} trig_itrigger_t;

typedef struct {
  uint64_t action : 6;  // [5:0]
  uint64_t u      : 1;  // [6]
  uint64_t s      : 1;  // [7]
  uint64_t pad0   : 1;  // [8]
  uint64_t m      : 1;  // [9]
  uint64_t pad1   : 1;  // [10]
  uint64_t vu     : 1;  // [11]
  uint64_t vs     : 1;  // [12]
  uint64_t pad2   : 45; // [57:13]
  uint64_t hit    : 1;  // [58]
  uint64_t dmode  : 1;  // [59] 
  uint64_t type   : 4;  // [63:60]
} trig_etrigger_t;

typedef struct {
  uint64_t action : 6;  // [5:0]
  uint64_t select : 16; // [21:6]
  uint64_t intctl : 1;  // [22]
  uint64_t pad    : 35; // [57:23]
  uint64_t hit    : 1;  // [58]
  uint64_t dmode  : 1;  // [59] 
  uint64_t type   : 4;  // [63:60]
} trig_tmexttrigger_t;

typedef struct {
  union {
    tdata1_t            common;
    trig_mcontrol_t     mcontrol;
    trig_mcontrol6_t    mcontrol6;
    trig_icount_t       icount;
    trig_itrigger_t     itrigger;
    trig_etrigger_t     etrigger;
    trig_tmexttrigger_t tmexttrigger;
    uint64_t            val;
  } tdata1;
  tdata2_t tdata2;
  tdata3_t tdata3;
} Trigger;

typedef struct TriggerModule {
  union {
    struct {
      uint64_t bf : 1; // before fetch
      uint64_t af : 1; // after fetch
      uint64_t br : 1; // before read
      uint64_t ar : 1; // after read
      uint64_t bw : 1; // before write
    };
    uint8_t val;
  } check_timings;
  // the last trigger is used to check maximum number of supported triggers
  Trigger triggers[CONFIG_TRIGGER_NUM + 1];
} TriggerModule;

void tm_update_timings(struct TriggerModule* TM);

trig_action_t tm_check_hit(struct TriggerModule* TM, trig_op_t op, vaddr_t addr, word_t data);

bool trigger_match(Trigger* trig, trig_op_t op, vaddr_t addr, word_t data);

bool trigger_value_match(Trigger* trig, word_t value);

void mcontrol_checked_write(trig_mcontrol_t* mcontrol, word_t* wdata, const struct TriggerModule* TM);

void trigger_handler(const trig_action_t action);

// Used to avoid magic number
#define TRIGGER_NO_VALUE (0)

#endif // __TRIGGER_H__
#endif // CONFIG_RVSDTRIG
