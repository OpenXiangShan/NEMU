#include "profiling/betapoint_profiling.h"

#include <common.h>
#include <debug.h>
#include <lz4.h>
#include <zstd.h>
#include <compress/zstd_compress_internal.h>
#include <cstdlib>

extern uint64_t g_nr_guest_instr;

namespace BetaPointNS {

CompressProfiler::CompressProfiler()
    : cctx(ZSTD_createCCtx()) {
    ZSTD_CCtx_setParameter(cctx, ZSTD_c_compressionLevel, 1);
    ZSTD_CCtx_setParameter(cctx, ZSTD_c_nbWorkers, 1);
    ZSTD_compressBegin(cctx, 1);
    outPtr = outputBuf;
}


ControlProfiler::ControlProfiler()
    : CompressProfiler(),
    brHist(brHistLen, 0) {
    info = reinterpret_cast<ControlInfo *>(inputBuf);
}

unsigned BetaPointNS::CompressProfiler::compressBlock(size_t input_size, char *in_ptr) {
    unsigned c_size = ZSTD_compressBlock(cctx, outPtr, contiguousBytes, in_ptr, input_size);
    return c_size;
}

void ControlProfiler::controlProfile(vaddr_t pc, vaddr_t target, bool taken) {
    // Logbeta("hist size: %lu", brHist.size());
    // fflush(stdout);
    Logbeta("pc: 0x%010lx, target: 0x%010lx, hist:0x%016lx,taken: %i", pc, target, brHist.to_ulong(), taken);
    fflush(stdout);
    info->pc = pc;
    info->target = target;
    info->taken = taken;
    info->hist = brHist.to_ulong();
    if (false) {
#define CMPBUFSIZE (LZ4_COMPRESSBOUND(_tokenBytes))
        char cmpBuf[CMPBUFSIZE];
        const int cmpBytes = LZ4_compress_fast_continue(lz4Stream, (char *)info, cmpBuf, tokenBytes(), CMPBUFSIZE, 0);
        Logbeta("lz cmp bytes: %i", cmpBytes);
        fflush(stdout);
    }
    if (false) {
        unsigned c_size = compressBlock(tokenBytes(), (char *)info);
        Logbeta("Br info zstd cSize: %u", c_size);
        fflush(stdout);
    }

    info++;
    if ((void*) info >= (void*) inputEnd) {
        info = reinterpret_cast<ControlInfo *>(inputBuf);
        Logbeta("flush br history dict");
    }

    // ppm
    if (false) {
        bool pred = ppm.lookup(pc, brHist);
        ppm.update(pc, brHist, taken, pred);
    }


    brHist = brHist << 1;
    brHist[0] = taken;
}

void ControlProfiler::onExit() {
    Log("PPM correct: %lu, PPM mispred: %lu", ppm.correct, ppm.mispred);
    Log("MPKI: %f", (double)ppm.mispred / (double) ::g_nr_guest_instr * 1000);
}

MemProfiler::MemProfiler()
    : CompressProfiler() {
    info = reinterpret_cast<MemInfo *>(inputBuf);
}

void MemProfiler::compressProfile(vaddr_t pc, vaddr_t vaddr, paddr_t paddr) {
    info->vaddr = vaddr;
    info->paddr = paddr;
    info->padding0 = 0;
    info->padding1 = 0;
    {
        unsigned c_size = compressBlock(tokenBytes(), (char *)info);
        Logbeta("Mem info zstd cSize: %u", c_size);
    }
    info++;
    if ((void*) info >= (void*) inputEnd) {
        info = reinterpret_cast<MemInfo *>(inputBuf);
        Logbeta("flush mem history dict");
    }
}

void MemProfiler::memProfile(vaddr_t pc, vaddr_t vaddr, paddr_t paddr) {
    Logbeta("vaddr: 0x%010lx, paddr: 0x%010lx", vaddr, paddr);
}

ControlProfiler ctrlProfiler;
MemProfiler memProfiler;

}


extern "C" {

void control_profile(vaddr_t pc, vaddr_t target, bool taken) {
    BetaPointNS::ctrlProfiler.controlProfile(pc, target, taken);
}

void control_on_exit() {
    BetaPointNS::ctrlProfiler.onExit();
}

}  // extern C

