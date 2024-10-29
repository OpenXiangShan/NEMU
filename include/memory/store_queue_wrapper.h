#ifndef NEMU_STORE_QUEUE_WRAPPER_H
#define NEMU_STORE_QUEUE_WRAPPER_H

#include <common.h>

#ifdef CONFIG_DIFFTEST_STORE_COMMIT
#include <memory/paddr.h>

#ifdef __cplusplus
extern "C" {
#endif

void store_queue_reset();
void store_queue_push(store_commit_t store_commit);
void store_queue_pop();
store_commit_t store_queue_fornt();
store_commit_t store_queue_back();
size_t store_queue_size();
bool store_queue_empty();

#ifdef __cplusplus
}
#endif

#endif //NEMU_STORE_QUEUE_WRAPPER_H
#endif //CONFIG_DIFFTEST_STORE_COMMIT
