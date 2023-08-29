#include <cinttypes>

#include <profiling/inst_profiling.h>
#include <profiling/betapoint_profiling.h>
#include <inst_trace/trace.h>

extern "C" {

void recordMem(uint64_t pc, uint64_t vaddr, uint64_t paddr, bool is_write)
{
#ifdef CONFIG_GEN_TRACE
    elasticTracer.recordMem(pc, paddr);
#endif
    BetaPointNS::memProfiler.memProfile(pc, vaddr, paddr, is_write);
}

void recordFetch(uint64_t pc, uint64_t vaddr, uint64_t inst_paddr)
{
#ifdef CONFIG_GEN_TRACE
    elasticTracer.recordFetch(pc, inst_paddr);
#endif
}

}