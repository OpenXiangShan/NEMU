/***************************************************************************************
* Copyright (c) 2026 Institute of Computing Technology, Chinese Academy of Sciences
*
* NEMU is licensed under Mulan PSL v2.
***************************************************************************************/

#ifndef __CPU_DIFFTEST_MEM_OBSERVATION_H__
#define __CPU_DIFFTEST_MEM_OBSERVATION_H__

#include <common.h>

#define DIFFTEST_MEM_OBSERVATION_VERSION_V1 1
#define DIFFTEST_MEM_OBSERVATION_MAX_SIZE_V1 16

enum DifftestMemObservationKindV1 {
  DIFFTEST_MEM_OBSERVATION_LOAD_V1 = 0,
  DIFFTEST_MEM_OBSERVATION_LR_V1 = 1,
};

enum DifftestMemObservationStateV1 {
  DIFFTEST_MEM_OBSERVATION_NONE_V1 = 0,
  DIFFTEST_MEM_OBSERVATION_PENDING_V1 = 1,
  DIFFTEST_MEM_OBSERVATION_CONSUMED_V1 = 2,
};

struct DifftestMemObservationV1 {
  uint16_t version;
  uint16_t struct_size;
  uint8_t kind;
  uint8_t size;
  uint16_t reserved;
  uint64_t paddr;
  uint8_t data[DIFFTEST_MEM_OBSERVATION_MAX_SIZE_V1];
};

_Static_assert(sizeof(struct DifftestMemObservationV1) == 32,
    "DifftestMemObservationV1 ABI changed");

#ifdef CONFIG_MULTICORE_DIFF
// One observation is installed immediately before difftest_exec(1). The
// instruction must consume the exact physical address and size; querying the
// state afterwards clears the observation.
int difftest_set_mem_observation_v1(
    const struct DifftestMemObservationV1 *observation);
int difftest_query_mem_observation_v1(void);
bool difftest_mem_observation_consume_v1(
    paddr_t paddr, int len, word_t *data);
#endif

#endif
