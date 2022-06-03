#include "profiling/betapoint_profiling.h"

#include <common.h>
#include <debug.h>
#include <lz4.h>
#include <zstd.h>
#include <compress/zstd_compress_internal.h>
#include <cstdlib>
#include <algorithm>

extern uint64_t g_nr_guest_instr;

namespace BetaPointNS {

extern DataflowProfiler dataflowProfiler;

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
    if (true) {
        bool pred = ppm.lookup(pc, brHist);
        calculateProbAndPenalty(pc, target, taken, pred);
        ppm.update(pc, brHist, taken, pred);
    }


    brHist = brHist << 1;
    brHist[0] = taken;
}

void ControlProfiler::onExit() {
    printf("PPM correct: %lu, PPM mispred: %lu\n", ppm.correct, ppm.mispred);
    printf("MPKI: %f\n", (double)ppm.mispred / (double) ::g_nr_guest_instr * 1000);
}

std::pair<float, unsigned> ControlProfiler::getProbAndPenalty(vaddr_t pc) {
    assert(pc == lastCtrlPC);
    return std::make_pair(lastCtrlMisProb, lastCtrlPenalty);
}

float ControlProfiler::getExpPenalty(vaddr_t pc) {
    Logbeta("pc: %lx, lastCtrl pc: %lx", pc, lastCtrlPC);
    assert(pc == lastCtrlPC);
    return lastPenaltyExpectation;
}

void ControlProfiler::calculateProbAndPenalty(vaddr_t pc, vaddr_t target, bool taken, bool ppm_pred) {
    if (ppm_pred != taken) {
        lastCtrlMisProb = 1.0;
        // todo: calculate real penalty with dataflow info?
        lastCtrlPenalty = DummyPenalty;
    } else {
        lastCtrlMisProb = 0.0;
        lastCtrlPenalty = 0;
    }
    lastPenaltyExpectation = lastCtrlPenalty * lastCtrlMisProb;
    lastCtrlPC = pc;
    Logbeta("Calc for pc 0x%010lx", pc);
}


MemProfiler::MemProfiler()
    : CompressProfiler() {
    info = reinterpret_cast<MemInfo *>(inputBuf);
    bitMap = roaring_bitmap_create_with_capacity(CONFIG_MSIZE/CacheBlockSize);
    for (unsigned i = 0; i < 4; i++) {
        distinctStrides[i] = roaring_bitmap_create_with_capacity(CONFIG_MSIZE/CacheBlockSize);
    }
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

void MemProfiler::memProfile(vaddr_t pc, vaddr_t vaddr, paddr_t paddr, bool is_write) {
    Logbeta("vaddr: 0x%010lx, paddr: 0x%010lx", vaddr, paddr);
    roaring_bitmap_add(bitMap, paddr / CacheBlockSize);
    globalStrideProfile(paddr, is_write);
    localStrideProfile(pc, paddr, is_write);
    reuseProfile(paddr);
}

void MemProfiler::globalStrideProfile(paddr_t paddr, int is_write){
    paddr_t last_addr = is_write ? lastWriteAddr: lastReadAddr;
    int64_t stride = int64_t(paddr - last_addr);
    if (globalStrides[is_write].find(stride) == globalStrides[is_write].end()) {
        globalStrides[is_write][stride] = 1;
    } else {
        globalStrides[is_write][stride] = globalStrides[is_write][stride] + 1;
    }
    lastReadAddr = is_write ? lastReadAddr: paddr;
    lastWriteAddr = is_write ? paddr: lastWriteAddr;
    roaring_bitmap_add(distinctStrides[GlobalStride * 2 + is_write], paddr / CacheBlockSize);
}

void MemProfiler::localStrideProfile(vaddr_t pc, paddr_t paddr, int is_write){
    if (localLastAddr[is_write].find(pc) == localLastAddr[is_write].end()) {
        localLastAddr[is_write][pc] = paddr;
    } else {
        paddr_t pre_addr = localLastAddr[is_write][pc];
        localLastAddr[is_write][pc] = paddr;
        int64_t stride = int64_t(paddr - pre_addr);
        auto pc_entry_it = localStrides[is_write].find(pc);

        if (pc_entry_it == localStrides[is_write].end()) {
            localStrides[is_write].emplace(pc, StrideCountMap());
            localStrides[is_write][pc][stride] = 1;
        } else {
            pc_entry_it->second[stride] += 1;
        }
    }
    roaring_bitmap_add(distinctStrides[LocalStride * 2 + is_write], paddr / CacheBlockSize);
}

std::array<unsigned, 4> &MemProfiler::getNewDistinctStride() {
    for (unsigned i = 0; i < 4; i++) {
        newDistinctStrides[i] = roaring_bitmap_get_cardinality(distinctStrides[i]) - lastCardinalities[i];
        lastCardinalities[i] += newDistinctStrides[i];
    }
    return newDistinctStrides;
}

void MemProfiler::dumpStride(int bucketSize){
    std::ofstream ofs;
    Log("Dump stride histogram");
    ofs.open("stride_histogram.csv");
    std::stringstream ss;

    for (int mem_type = 0; mem_type < 2; mem_type++){
        // if (mem_type == 0) {
        //     ss << "global read stride:" << std::endl;
        // } else {
        //     ss << "global write stride:" << std::endl;
        // }
        auto iter = globalStrides[mem_type].begin();
        auto &global_stride_bucket = globalStrideBuckets[mem_type];
        for (auto cursor: bucketRanges) {
            while (iter != globalStrides[mem_type].end() && iter->first < cursor) {
                if (global_stride_bucket.find(iter->first) == global_stride_bucket.end()) {
                    global_stride_bucket[cursor] = iter->second;
                } else {
                    global_stride_bucket[cursor] += + iter->second;
                }
                iter++;
            }
            if (iter == globalStrides[mem_type].end()) {
                break;
            }
        }
        for (auto iter = globalStrideBuckets[mem_type].begin(); iter != globalStrideBuckets[mem_type].end(); ++iter){
            // ss << "~" << iter->first << ": " << iter->second << std::endl;
            ss << iter->second << ",";
        }
        ss << 0 << std::endl;
    }

    for (int mem_type = 0; mem_type < 2; mem_type++){
        // if (mem_type == 0) {
        //     ss << "local read stride:" << std::endl;
        // } else {
        //     ss << "local write stride:" << std::endl;
        // }
        auto &local_stride_bucket = localStrideBuckets[mem_type];
        for (auto cursor: bucketRanges) {
            for (auto pc_stride_pair: localStrides[mem_type]) {
                auto &stride_count_map = pc_stride_pair.second;
                auto stride_count_pair = stride_count_map.begin();
                while (stride_count_pair != stride_count_map.end() && stride_count_pair->first < cursor) {
                    if (local_stride_bucket.find(cursor) == local_stride_bucket.end()) {
                        local_stride_bucket[cursor] = stride_count_pair->second;
                    } else {
                        local_stride_bucket[cursor] += stride_count_pair->second;
                    }
                    stride_count_pair++;
                }
                if (stride_count_pair == stride_count_map.end()) {
                    break;
                }
            }
        }
        for (auto iter = localStrideBuckets[mem_type].begin(); iter != localStrideBuckets[mem_type].end(); ++iter){
            // ss << "~" << iter->first << ": " << iter->second << std::endl;
            ss << iter->second << ",";
        }
        ss << 0 << std::endl;
    }

    ofs << ss.rdbuf();
    ofs.close();
}

void MemProfiler::reuseProfile(paddr_t paddr) {
    uint64_t icount = dataflowProfiler.getProfiledInsts();
    if (icount >= nextNewChunkInsts) {
        nextNewChunkInsts += reuseChunkSize;
        reuseBitMaps.push_back(roaring_bitmap_create_with_capacity(numCacheBlocks));
    }
    auto &reuse_bitmap = reuseBitMaps.back();
    roaring_bitmap_add(reuse_bitmap, paddr / CacheBlockSize);
}

void MemProfiler::calcReuseMatrix() {
    reuseMatrix.resize(reuseBitMaps.size(), std::vector<double>(reuseBitMaps.size(), 0.0));
    for (unsigned i = 0; i < reuseBitMaps.size(); i++) {
        for (unsigned j = i + 1; j < reuseBitMaps.size(); j++) {
            reuseMatrix[i][j] = (double) roaring_bitmap_and_cardinality(reuseBitMaps[i], reuseBitMaps[j]) /
                    (double) roaring_bitmap_get_cardinality(reuseBitMaps[i]);
        }
    }
    Log("Dump Reuse matrix");
    std::ofstream ofs;
    ofs.open("reuse_matrix.csv");
    for (unsigned i = 0; i < reuseBitMaps.size(); i++) {
        for (unsigned j = 0; j < reuseBitMaps.size(); j++) {
            ofs << reuseMatrix[i][j] << ",";
        }
        ofs << 0 << std::endl;
    }
}

void MemProfiler::chunkEnd() {
    auto inc = (int64_t) memProfiler.getFootprint() - lastFootprintSize;
    if (inc >= 0) {
        footprintIncrements.push_back(inc);
        lastFootprintSize = memProfiler.getFootprint();
    } else {
        xpanic("Footprint decrease: %ld", inc);
    }

    const auto &new_distinct_strides = getNewDistinctStride();
    newDistinctStrideHist.push_back(new_distinct_strides);
}

void MemProfiler::onExit() {
    uint32_t cardinality = roaring_bitmap_get_cardinality(bitMap);
    printf("Footprint: %u cacheblocks\n", cardinality);
    printf("dump stride\n");
    dumpStride(6);
    calcReuseMatrix();
    for (auto &reuse_bitmap: reuseBitMaps) {
        roaring_bitmap_free(reuse_bitmap);
    }

    std::ofstream outf;

    Log("Dump footprint increments: %lu", footprintIncrements.size());
    outf.open(std::string(outputDir) + "new_footprints.csv", std::ios::out);
    for (const auto &x: footprintIncrements) {
        outf << x << ",";
    }
    outf << 0 << std::endl;
    outf.close();

    Log("Dump new distinct strides : %lu", newDistinctStrideHist.size());
    outf.open(std::string(outputDir) + "new_strides.csv", std::ios::out);
    for (unsigned i = 0; i < 4; i++) {
        for (const auto &x: newDistinctStrideHist) {
            outf << x[i] << ",";
        }
        outf << 0 << std::endl;
    }
    outf.close();
}

DataflowProfiler::DataflowProfiler() {
    ppmMisPreds.push_back(0);
}

void DataflowProfiler::dataflowProfile(vaddr_t pc, paddr_t paddr, bool is_store, uint8_t mem_width,
        uint8_t dst_id, uint8_t src1_id, uint8_t src2_id, uint8_t fsrc3_id, uint8_t is_ctrl) {
    profiledInsts++;
    if (mem_width != 0) {
        for (paddr_t baddr = paddr; baddr < paddr + mem_width; baddr++) {
            auto it = memDepMap.find(baddr);
            if (it != memDepMap.end()) {
                auto record = it->second;
                if (is_store) {
                    // Store: override the dep length and mem width
                    unsigned longest_dep_len = std::max(regDepMap[src1_id], regDepMap[src2_id]);
                    record.depLen = longest_dep_len + storeLatency;
                    record.ssn = storeSN;
                    record.paddr = paddr;
                    Logmp("Store dep len: %u", record.depLen);
                } else {
                    // Load: propergate dep length to register 
                    assert(dst_id != 0);
                    regDepMap[dst_id] = record.depLen + loadLatency;
                    Logmp("Load inc store dep len: %u", record.depLen);
                }
            } else {
                if (is_store) {
                    // create new entry
                    unsigned longest_dep_len = std::max(regDepMap[src1_id], regDepMap[src2_id]);
                    // auto &new_record = memRecordList.emplace_back(storeSN, paddr, longest_dep_len);
                    memDepMap.emplace(std::make_pair(baddr, MemDepRecord(storeSN, paddr, longest_dep_len)));
                } else {
                    assert(dst_id != 0);
                    regDepMap[dst_id] = 0;
                }
            }
        }
        if (is_store) {
            storeSN++;
            // inFlightStoreCount++;
            // if (likely(inFlightStoreCount >= storeQueueSize)) {
            //     if (storeSN % 1000 == 0) {
            //         Log("store in flight size: %u", inFlightStoreCount);
            //     }
            //     auto front_ssn = memRecordList.front().ssn;
            //     auto it = memRecordList.begin();
            //     while (it != memRecordList.end() && it->ssn <= front_ssn) {
            //         memDepMap.erase(it->paddr);
            //         it = memRecordList.erase(it);
            //     }
            //     inFlightStoreCount--;
            // }
        }
    } else {
        if (!is_ctrl) {
            unsigned longest_dep_len = std::max(std::max(regDepMap[src1_id], regDepMap[src2_id]), regDepMap[fsrc3_id]);
            regDepMap[dst_id] = longest_dep_len + 1; //todo: fix to real latency
        } else {
            // control instruction universally increase the path by its flush cycle expectation
            float penalty = ctrlProfiler.getExpPenalty(pc);
            if (penalty > 0.01) {
                ppmMisPreds.back()++;
                for (auto &it: regDepMap) {
                    if (it > 0) {
                        it += penalty;
                    }
                }
                for (auto &it: memDepMap) {
                    if (it.second.depLen > 0) {
                        it.second.depLen += penalty;
                    }
                }
                Logcp("Add mispred exp penalty: %f", penalty);
            }
        }
    }
    regDepMap[0] = 0; // always set it to zero to simplify logic
    inFlightInstCount++;
    if (inFlightInstCount >= instWindowSize) {
        // append critical path
        auto m = std::max_element(regDepMap.begin(), regDepMap.end());
        criticalPathLen.push_back(*m);

        // start new ppm miss counter
        ppmMisPreds.push_back(0);

        // count footprint increase

        memProfiler.chunkEnd();

        // Log("critical path: %u", *m);
        // clear
        inFlightInstCount = 0;
        clearRegDepMap();
        clearMemDepMap();
    }
}

void DataflowProfiler::onExit() {
    std::ofstream outf;

    Log("Dump critical path size: %lu", criticalPathLen.size());
    outf.open(std::string(outputDir) + "critical_paths.csv", std::ios::out);
    for (const auto &x: criticalPathLen) {
        outf << x << ",";
    }
    outf << 0 << std::endl;
    outf.close();

    Log("Dump ppm miss count: %lu", ppmMisPreds.size());
    outf.open(std::string(outputDir) + "ppm_mispreds.csv", std::ios::out);
    for (const auto &x: ppmMisPreds) {
        outf << x << ",";
    }
    outf << 0 << std::endl;
    outf.close();


}

ControlProfiler ctrlProfiler;
MemProfiler memProfiler;
DataflowProfiler dataflowProfiler;

const char *outputDir = "./";
}


extern "C" {

void control_profile(vaddr_t pc, vaddr_t target, bool taken) {
    BetaPointNS::ctrlProfiler.controlProfile(pc, target, taken);
}

void dataflow_profile(vaddr_t pc, paddr_t paddr, bool is_store, uint8_t mem_width,
        uint8_t dst_id, uint8_t src1_id, uint8_t src2_id, uint8_t fsrc3_id, uint8_t is_ctrl) {
    BetaPointNS::dataflowProfiler.dataflowProfile(pc, paddr, is_store, mem_width,
        dst_id, src1_id, src2_id, fsrc3_id, is_ctrl);
}

void beta_on_exit() {
    BetaPointNS::ctrlProfiler.onExit();
    BetaPointNS::memProfiler.onExit();
    BetaPointNS::dataflowProfiler.onExit();
}

}  // extern C

