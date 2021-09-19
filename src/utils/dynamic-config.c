#include <common.h>

struct DynamicConfig dynamic_config = {};
void update_dynamic_config(void* config) {
  memcpy((void*)&dynamic_config, config, sizeof(struct DynamicConfig));
}
