#ifdef CONFIG_RV_IMSIC

#ifndef __AIA_H__
#define __AIA_H__

#include "common.h"
#include "csr.h"
#include "intr.h"

// 0,4,8 is reserved
#define IPRIO_ENABLE_NUM 61
#define IPRIO_NUM 8

typedef struct {
  word_t val;
} Iprio;

typedef struct IpriosModule {
  Iprio iprios[IPRIO_NUM];
} IpriosModule;

typedef struct {
  bool enable;
  bool isZero;
  bool greaterThan255;
  uint8_t priority;
} IpriosEnable;

typedef struct IpriosSort {
  IpriosEnable ipriosEnable[IPRIO_ENABLE_NUM];
} IpriosSort;

typedef struct {
  uint8_t hviprios[24];
} Hviprios;

void set_iprios_sort(uint64_t topi_gather, IpriosSort* iprios_sort, IpriosModule* iprios, uint8_t xei, mtopei_t* xtopei);
void set_viprios_sort(uint64_t topi_gather);
uint8_t high_iprio(IpriosSort* ipriosSort, uint8_t xei);
uint8_t get_prio_idx_in_group(uint8_t irq);

extern int interrupt_default_prio[IPRIO_ENABLE_NUM];

bool no_mtopi();
bool no_stopi();
bool no_vstopi();

#endif // __AIA_H__
#endif // CONFIG_RV_IMSIC