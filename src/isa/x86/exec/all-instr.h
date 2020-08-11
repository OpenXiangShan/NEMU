#include "../local-include/rtl.h"

#include "arith.h"
#include "control.h"
#include "data-mov.h"
#include "logic.h"
#include "string.h"
#include "system.h"
#include "eflags.h"
#include "bit.h"
#include "misc.h"
#include "vector.h"
#include "fp.h"

def_EHelper(nop);
def_EHelper(inv);
def_EHelper(nemu_trap);
