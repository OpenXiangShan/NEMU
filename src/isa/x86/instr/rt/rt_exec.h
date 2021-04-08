#define def_REHelper(name) static inline void concat(rt_exec_, name) (Decode *s, int width)

#define def_DEWBWHelper(name, id, exec, wb, width) \
  def_EHelper(name) { \
    concat(rt_decode_, id) (s, concat(WIDTH_, width)); \
    concat(rt_exec_, exec) (s, concat(WIDTH_, width)); \
    concat(rt_wb_, wb) (s, concat(WIDTH_, width)); \
  }

#define def_EWBWHelper(name, exec, wb, width) \
  def_EHelper(name) { \
    concat(rt_exec_, exec) (s, concat(WIDTH_, width)); \
    concat(rt_wb_, wb) (s, concat(WIDTH_, width)); \
  }

#define def_DEWHelper(name, id, exec, width) \
  def_EHelper(name) { \
    concat(rt_decode_, id) (s, concat(WIDTH_, width)); \
    concat(rt_exec_, exec) (s, concat(WIDTH_, width)); \
  }

#define def_EWHelper(name, exec, width) \
  def_EHelper(name) { \
    concat(rt_exec_, exec) (s, concat(WIDTH_, width)); \
  }

#include "rt_data-mov.h"
#include "rt_arith.h"
#include "rt_logic.h"
#include "rt_string.h"
#include "rt_bit.h"
#include "rt_system.h"
