#undef def_EHelper
#define finish_label exec_finish
#define def_label(l) l:
#define def_EHelper(name) \
  goto finish_label; /* this is for the previous def_EHelper() */ \
  def_label(name)

#define def_start()
#define def_finish() def_label(finish_label)

#include "special.h"
#include "compute.h"
#include "control.h"
#include "ldst.h"
#include "muldiv.h"
#include "system.h"
