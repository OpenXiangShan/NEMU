#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

typedef uint8_t nemu_bool;
#define true 1
#define false 0

typedef uint32_t paddr_t;

#include "protocol.h"

#endif
