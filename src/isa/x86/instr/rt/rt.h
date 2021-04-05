#ifndef __RT_H__
#define __RT_H__

enum {
  WIDTH_dynamic = 0,
  WIDTH_b = 1,
  WIDTH_w = 2,
  WIDTH_l = 4,
};

#include "cc.h"

#include "rt_decode.h"
#include "rt_exec.h"
#include "rt_wb.h"

#endif
