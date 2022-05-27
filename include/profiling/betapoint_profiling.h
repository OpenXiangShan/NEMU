#include <profiling/ppm.h>
#include <common.h>
#include <debug.h>
#include <checkpoint/cpt_env.h>
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
#include <limits>

namespace BetaPointNS {

#define statChunkSize (1024 * 1024)

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

    const char *outputDir() {
        return output_base_dir;
    }
    
  public:
    virtual void onStart() {};
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

    const unsigned CacheBlockSize{64};
    const unsigned PageSize{4096};

    ///////////////////////////////////////////
    // Basic
    uint64_t numLoads, numStores;

    ///////////////////////////////////////////
    // Footprint
    roaring_bitmap_t *bitMap;

    uint64_t lastFootprintSize{0};
    std::vector<unsigned> footprintIncrements;

    ///////////////////////////////////////////
    // PC -> access count
    std::array<std::map<vaddr_t, int64_t>, 2> localAccessCount;
    std::ofstream localAccessCountFile;

    ///////////////////////////////////////////
    // Strides
    using StrideCountMap = std::map<int64_t, int64_t>;

    enum {
        GlobalStride = 0,
        LocalStride
    };

    // store last memory access address corresponding to pc
    std::array<std::map<vaddr_t, paddr_t>, 2> localLastAddr;

    // store in-flight global stride and local stride
    std::array<StrideCountMap, 2> globalStrides;
    std::array<std::map<vaddr_t, StrideCountMap>, 2> localStrides;

    // store global stride and local stride
    std::array<StrideCountMap, 2> globalStrideBuckets;
    std::array<StrideCountMap, 2> localStrideBuckets;

    paddr_t lastReadAddr, lastWriteAddr;

    const unsigned numHalfBuckets{16};
    const int bucketStart{4};
    std::vector<int64_t> bucketRanges;

    std::array<roaring_bitmap_t *, 4> distinctStrides;

    std::array<unsigned, 4> lastCardinalities{0, 0, 0, 0};
    std::array<unsigned, 4> newDistinctStrides{0, 0, 0, 0};
    std::array<unsigned, 4> &getNewDistinctStride();
    std::vector<std::array<unsigned, 4>> newDistinctStrideHist;

    ///////////////////////////////////////////
    // Reuse
    // uint64_t numPages{(uint64_t)CONFIG_MSIZE/PageSize};
    uint64_t numCacheBlocks{(uint64_t)CONFIG_MSIZE/CacheBlockSize};
    std::vector<roaring_bitmap_t *> reuseBitMaps;
    uint64_t reuseChunkSize{2500};
    uint64_t nextNewChunkInsts{0};
    std::vector<std::vector<double>> reuseMatrix;

    void reuseProfile(paddr_t paddr);
    void calcReuseMatrix();

    const bool StrideVerbose{false};
    
  public:
    MemProfiler();

    void memProfile(vaddr_t pc, vaddr_t vaddr, paddr_t paddr, bool is_write);
    void globalProfile(paddr_t paddr, int is_write);
    void localProfile(vaddr_t pc, paddr_t paddr, int is_write);
    void compressProfile(vaddr_t pc, vaddr_t vaddr, paddr_t paddr);

    void dumpStride();
    void dumpFootPrintInc();
    void dumpDistinctStrideInc();
    void dumpReuseMatrix();

    std::array<StrideCountMap, 2> &getGlobalStrides() {
        return globalStrideBuckets;
    }
    std::array<StrideCountMap, 2> &getLocalStrides() {
        return localStrideBuckets;
    }

    uint64_t getFootprint() const {
        return roaring_bitmap_get_cardinality(bitMap);
    }

    void chunkEnd();

    void computeTopAccesses();

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

    uint64_t profiledInsts{};

    std::vector<unsigned> criticalPathLen;

    std::vector<unsigned> ppmMisPreds;

    std::ofstream chunkStatsStream;

    const char *outputDir() {
        return output_base_dir;
    }
  public:
    DataflowProfiler();

    uint64_t getProfiledInsts() const {
        return profiledInsts;
    }

    void onStart();

    void onExit();

    void dataflowProfile(vaddr_t pc, paddr_t paddr, bool is_store, uint8_t mem_width,
        uint8_t dst_id, uint8_t src1_id, uint8_t src2_id, uint8_t fsrc3_id, u_int8_t is_ctrl);

    std::ofstream &getChunkStatsStream() {
        return chunkStatsStream;
    }
};

}