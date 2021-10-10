#include <common.h>

struct DynamicConfig dynamic_config = {};
void update_dynamic_config(void* config) {
  memcpy((void*)&dynamic_config, config, sizeof(struct DynamicConfig));
  printf("NEMU dynamic_config update\n");
  printf("  - ignore_illegal_mem_access %x\n", dynamic_config.ignore_illegal_mem_access);
  printf("  - debug_difftest %x\n", dynamic_config.debug_difftest);
}
