#include <profiling/ppm.h>
#include <common.h>
#include <debug.h>
#include <lz4.h>
#include <zstd.h>
#include <roaring/roaring.h>

#include <array>
#include <list>
#include <unordered_map>
#include <boost/dynamic_bitset.hpp>
#include <boost/utility/binary.hpp>

#include <map>
#include <iostream>
#include <vector>
#include <fstream>
#include <utility>

namespace BetaPointNS {

class CompressProfiler {
  protected:
    static const unsigned bufBytes = 64;
    static const unsigned contiguousTokens = 1024 * 1024;
    static const unsigned contiguousBytes = bufBytes*contiguousTokens;

    char inputBuf[contiguousBytes];
    char *inputEnd = &inputBuf[contiguousBytes + 1];
    char outputBuf[contiguousBytes];
    char *outPtr;

    LZ4_stream_t lz4StreamBody{ {0} };
    LZ4_stream_t *lz4Stream {&lz4StreamBody};

    ZSTD_CCtx* const cctx;

    CompressProfiler();

    unsigned compressBlock(size_t input_size, char *in_ptr);

    virtual unsigned tokenBytes() = 0;

    virtual void onExit() = 0;
};

class ControlProfiler: public CompressProfiler {
    struct ControlInfo {
        vaddr_t pc;
        vaddr_t target;
        uint64_t hist;
        bool taken;
    };
    ControlInfo *info;

    unsigned _tokenBytes = sizeof(ControlInfo);
    unsigned tokenBytes() override {
        return _tokenBytes;
    }

    const unsigned brHistLen{64};
    boost::dynamic_bitset<> brHist;

    PPMNS::PPM ppm;

    const unsigned DummyPenalty{128};

    paddr_t lastCtrlPC;
    unsigned lastCtrlPenalty;
    float lastCtrlMisProb;
    float lastPenaltyExpectation;

  public:
    ControlProfiler();

    void controlProfile(vaddr_t pc, vaddr_t target, bool taken);

    void onExit() override;

    void calculateProbAndPenalty(vaddr_t pc, vaddr_t target, bool taken, bool ppm_pred);

    std::pair<float, unsigned> getProbAndPenalty(vaddr_t pc);
    float getExpPenalty(vaddr_t pc);
};

extern ControlProfiler ctrlProfiler;

class MemProfiler: public CompressProfiler {
    struct MemInfo{
        // vaddr_t pc;
        vaddr_t vaddr;
        paddr_t paddr;
        paddr_t padding0;
        paddr_t padding1;
    };

    MemInfo *info;

    unsigned _tokenBytes = sizeof(MemInfo);
    unsigned tokenBytes() override {
        return _tokenBytes;
    }

    roaring_bitmap_t *bitMap;

    // store memory address corresponding to pc
    std::vector<std::map<vaddr_t, paddr_t>> localMap;
    // store temporary global stride and local stride
    std::vector<std::map<int64_t, int>> tempGlobalStride;
    std::vector<std::map<vaddr_t, std::vector<std::pair<int64_t, int>>>> tempLocalStride;
    // store global stride and local stride
    std::vector<std::map<int64_t, int>> globalStride;
    std::vector<std::map<int64_t, int>> localStride;

    const unsigned CacheBlockSize{64};

    paddr_t lastReadAddr, lastWriteAddr;

  public:
    MemProfiler();

    void memProfile(vaddr_t pc, vaddr_t vaddr, paddr_t paddr, bool is_write);
    void globalStrideProfile(paddr_t paddr, int is_write);
    void localStrideProfile(vaddr_t pc, paddr_t paddr, int is_write);
    void compressProfile(vaddr_t pc, vaddr_t vaddr, paddr_t paddr);
    void dumpStride(int bucketSize);
    std::vector<std::map<int64_t, int>> &getGlobalStrides() {
        return globalStride;
    }
    std::vector<std::map<int64_t, int>> &getLocalStrides() {
        return localStride;
    }

    void onExit() override;
};

extern MemProfiler memProfiler;

class DataflowProfiler {
    struct MemDepRecord {
        uint64_t ssn;
        paddr_t paddr;
        unsigned depLen;

        MemDepRecord(uint64_t ssn, paddr_t paddr, unsigned depLen) : ssn(ssn), paddr(paddr), depLen(depLen) {}
    };
    // std::list<MemDepRecord> memRecordList;
    std::unordered_map<paddr_t, MemDepRecord> memDepMap;
    void clearMemDepMap() {
        memDepMap.clear();
    }

    uint64_t storeSN{0};
    // unsigned inFlightInstCount{0};
    unsigned inFlightInstCount{0};
    unsigned inFlightStoreCount{0};
    const unsigned storeQueueSize{72};
    const unsigned instWindowSize{512};

    std::array<unsigned, 64> regDepMap;
    void clearRegDepMap() {
        for (auto &regDep : regDepMap) {
            regDep = 0;
        }
    }

    const unsigned storeLatency{1}, loadLatency{1},
            divLatency{1}, controlLatency{1}, addMulLatency{1};

  public:
    void dataflowProfile(vaddr_t pc, paddr_t paddr, bool is_store, uint8_t mem_width,
        uint8_t dst_id, uint8_t src1_id, uint8_t src2_id, uint8_t fsrc3_id, u_int8_t is_ctrl);
};

}