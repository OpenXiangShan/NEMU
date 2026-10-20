#include <common.h>
#ifdef CONFIG_RV_AME
#include "mvalue.h"
def_EHelper(mew) {
  ame_ew_execute(s);
}
def_EHelper(mlv) {
  ame_lv_memory(s);
}
#endif
