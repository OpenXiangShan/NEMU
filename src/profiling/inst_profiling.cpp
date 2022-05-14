#include <cinttypes>

#include <profiling/inst_profiling.h>
#include <inst_trace/trace.h>

extern "C" {

void recordMem(uint64_t pc, uint64_t paddr)
{
#ifdef CONFIG_GEN_TRACE
    elasticTracer.recordMem(pc, paddr);
#endif
}

void recordFetch(uint64_t pc, uint64_t inst_paddr)
{
#ifdef CONFIG_GEN_TRACE
    elasticTracer.recordFetch(pc, inst_paddr);
#endif
}

}