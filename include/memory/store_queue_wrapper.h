#ifndef NEMU_STORE_QUEUE_WRAPPER_H
#define NEMU_STORE_QUEUE_WRAPPER_H

#include <common.h>
#include <memory/paddr.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_STORE_LOG
void store_log_stack_reset();
void store_log_stack_push(store_log_t log);
void store_log_stack_pop();
store_log_t store_log_stack_top();
bool store_log_stack_empty();
#ifdef CONFIG_LIGHTQS
void spec_store_log_stack_reset();
void spec_store_log_stack_push(store_log_t log);
void spec_store_log_stack_pop();
store_log_t spec_store_log_stack_top();
bool spec_store_log_stack_empty();
void spec_store_log_stack_copy();
#endif // CONFIG_LIGHTQS
#endif // CONFIG_STORE_LOG

#ifdef CONFIG_DIFFTEST_STORE_COMMIT
void store_queue_reset();
void store_queue_push(store_commit_t store_commit);
void store_queue_pop();
store_commit_t store_queue_front();
store_commit_t store_queue_back();
size_t store_queue_size();
bool store_queue_empty();
#endif //CONFIG_DIFFTEST_STORE_COMMIT

#ifdef __cplusplus
}
#endif

#endif //NEMU_STORE_QUEUE_WRAPPER_H
