#include "fp-include.h"

def_EHelper(faddp) {
  rtl_faddd(s, dfdest, dfdest, dfsrc1);
  ftop_pop();
}
