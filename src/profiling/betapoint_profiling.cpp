#include "betapoint_profiling.h"

#include <common.h>
#include <debug.h>
#include <lz4.h>
#include <zstd.h>
#include <compress/zstd_compress_internal.h>
#include <cstdlib>

namespace BetaPointNS {

CompressProfiler::CompressProfiler()
    : cctx(ZSTD_createCCtx()) {}


ControlProfiler::ControlProfiler()
    : CompressProfiler(),
    brHist(brHistLen, 0) {
    ZSTD_CCtx_setParameter(cctx, ZSTD_c_compressionLevel, 1);
    ZSTD_CCtx_setParameter(cctx, ZSTD_c_nbWorkers, 1);
    ZSTD_compressBegin(cctx, 1);
    // Logbeta("hist size: %lu", brHist.size());
    info = reinterpret_cast<ControlInfo *>(inputBuf);
    outPtr = outputBuf;
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
    info++;
    if ((void*) info >= (void*) inputEnd) {
        info = reinterpret_cast<ControlInfo *>(inputBuf);
    }
    info->pc = pc;
    info->target = target;
    info->taken = taken;
    info->hist = brHist.to_ulong();
    {
#define CMPBUFSIZE (LZ4_COMPRESSBOUND(_tokenBytes))
        char cmpBuf[CMPBUFSIZE];
        const int cmpBytes = LZ4_compress_fast_continue(lz4Stream, (char *)info, cmpBuf, tokenBytes(), CMPBUFSIZE, 0);
        Logbeta("lz cmp bytes: %i", cmpBytes);
        fflush(stdout);
    }
    {
        unsigned c_size = compressBlock(tokenBytes(), (char *)info);
        Logbeta("This cSize: %u", c_size);
        fflush(stdout);
    }
    brHist = brHist << 1;
    brHist[0] = taken;
}

ControlProfiler ctrlProfiler;

}

extern "C" {

void control_profile(vaddr_t pc, vaddr_t target, bool taken) {
    BetaPointNS::ctrlProfiler.controlProfile(pc, target, taken);
}

}  // extern C