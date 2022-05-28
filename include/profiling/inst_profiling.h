#ifndef __INST_PROFILING_H__
#define __INST_PROFILING_H__

#ifdef __cplusplus
extern "C" {
#endif

void recordMem(uint64_t pc, uint64_t vaddr, uint64_t paddr, bool is_write);
void recordFetch(uint64_t pc, uint64_t vaddr, uint64_t inst_paddr);

#ifdef __cplusplus
}
#endif

#endif