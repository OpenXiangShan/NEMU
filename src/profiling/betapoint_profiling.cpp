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
    bitMap = roaring_bitmap_create_with_capacity(CONFIG_MSIZE/64);
    std::map<int64_t, int> vecGlobalMap;
    std::map<vaddr_t, paddr_t> vecLocalMap;
    std::map<vaddr_t, std::vector<std::pair<int64_t, int> > > veclocalStride;
    for(int mem_type = 0;mem_type < 2;mem_type++){
        tempGlobalStride.push_back(vecGlobalMap);
        globalStride.push_back(vecGlobalMap);
        localStride.push_back(vecGlobalMap);
        localMap.push_back(vecLocalMap);
        tempLocalStride.push_back(veclocalStride);
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
}

void MemProfiler::globalStrideProfile(paddr_t paddr, int is_write){
    paddr_t last_addr = is_write ? lastWriteAddr: lastReadAddr;
    int64_t stride = int64_t(paddr - last_addr);
    if (tempGlobalStride[is_write].find(stride) == tempGlobalStride[is_write].end()) {
        tempGlobalStride[is_write][stride] = 1;
    } else {
        tempGlobalStride[is_write][stride] = tempGlobalStride[is_write][stride] + 1;
    }
    lastReadAddr = is_write ? lastReadAddr: paddr;
    lastWriteAddr = is_write ? paddr: lastWriteAddr;
}

void MemProfiler::localStrideProfile(vaddr_t pc, paddr_t paddr, int is_write){
    if (localMap[is_write].find(pc) == localMap[is_write].end()) {
        localMap[is_write][pc] = paddr;
    } else {
        paddr_t pre_addr = localMap[is_write][pc];
        localMap[is_write][pc] = paddr;
        int64_t stride = int64_t(paddr - pre_addr);
        if (tempLocalStride[is_write].find(pc) == tempLocalStride[is_write].end()) {
            std::vector<std::pair<int64_t, int>> vec;
            vec.push_back(std::make_pair(stride, 1));
            tempLocalStride[is_write].insert(std::pair<vaddr_t, std::vector<std::pair<int64_t, int>>>(pc, vec));
        } else {
            for(std::vector<std::pair<int64_t, int> >::iterator iter = tempLocalStride[is_write][pc].begin();iter != tempLocalStride[is_write][pc].end();++iter){
                if (iter->first == stride) {
                    iter->second++;
                    break;
                }
            }
        }
    }
}

void MemProfiler::dumpStride(int bucketSize){
    std::ofstream ofs;
    ofs.open("strideHistogram.txt");
    std::stringstream ss;

    for(int mem_type = 0;mem_type < 2;mem_type++){
        if (mem_type == 0) {
            ss << "global read stride:" << std::endl;
        } else {
            ss << "global write stride:" << std::endl;
        }
        for(auto iter = tempGlobalStride[mem_type].begin();iter != tempGlobalStride[mem_type].end();++iter){
            int64_t index = iter->first < 0 ? iter->first / bucketSize - 1 : iter->first / bucketSize;
            auto &global_stride = globalStride[mem_type];
            if (global_stride.find(index) == global_stride.end()) {
                global_stride[index] = iter->second;
            } else {
                global_stride[index] = global_stride[index] + iter->second;
            }
        }
        for(auto iter = globalStride[mem_type].begin();iter != globalStride[mem_type].end();++iter){
            if (iter->first >= 0) {
                ss << std::hex << iter->first * bucketSize << "~" << (iter->first + 1) * bucketSize - 1 << " " << std::oct << iter->second << std::endl;
            } else {
                ss << std::hex << "-" << -iter->first * bucketSize << "~-" << (-iter->first - 1) * bucketSize + 1 << " " << std::oct << iter->second << std::endl;
            }
        }
    }

    for(int mem_type = 0;mem_type < 2;mem_type++){
        if (mem_type) {
            ss << "local read stride:" << std::endl;
        } else {
            ss << "local write stride:" << std::endl;
        }
        for(auto iter = tempLocalStride[mem_type].begin();iter != tempLocalStride[mem_type].end();++iter){
            std::vector<std::pair<int64_t, int> > vec = iter->second;
            for(std::vector<std::pair<int64_t, int> >::iterator vecIter = vec.begin();vecIter != vec.end();++vecIter){
                int64_t index = vecIter->first < 0 ? vecIter->first / bucketSize - 1 : vecIter->first / bucketSize;
                auto &local_stride = localStride[mem_type];
                if (local_stride.find(index) == local_stride.end()) {
                    local_stride[index] = vecIter->second;
                } else {
                    local_stride[index] = local_stride[index] + vecIter->second;
                }
            }
        }
        for(auto iter = localStride[mem_type].begin();iter != localStride[mem_type].end();++iter){
            if (iter->first >= 0) {
                ss << std::hex << iter->first * bucketSize << "~" << (iter->first + 1) * bucketSize - 1 << " " << std::oct << iter->second << std::endl;
            } else {
                ss << std::hex << "-" << -iter->first * bucketSize << "~-" << (-iter->first - 1) * bucketSize + 1 << " " << std::oct << iter->second << std::endl;
            }
        }
    }

    ofs << ss.rdbuf();
    ofs.close();
}

void MemProfiler::onExit() {
    uint32_t cardinality = roaring_bitmap_get_cardinality(bitMap);
    printf("Footprint: %u cacheblocks\n", cardinality);
    printf("dump stride\n");
    dumpStride(6);
}


void DataflowProfiler::dataflowProfile(vaddr_t pc, paddr_t paddr, bool is_store, uint8_t mem_width,
        uint8_t dst_id, uint8_t src1_id, uint8_t src2_id, uint8_t fsrc3_id, uint8_t is_ctrl) {
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
        // dump critical path
        auto m = std::max_element(regDepMap.begin(), regDepMap.end());
        Log("critical path: %u", *m);
        // clear
        inFlightInstCount = 0;
        clearRegDepMap();
        clearMemDepMap();
    }
}

ControlProfiler ctrlProfiler;
MemProfiler memProfiler;
DataflowProfiler dataflowProfiler;

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
}

}  // extern C

