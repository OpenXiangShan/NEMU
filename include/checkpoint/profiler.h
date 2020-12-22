#ifndef __SIAMESE_PROFILER__
#define __SIAMESE_PROFILER__


#include <cinttypes>
#include <unordered_map>
#include <unordered_set>

#include <base/output.h>
#include <base/sized_ranker.h>
#include <cpu/decode.h>
#include <list>


class SiameseProfiler
{
  public:
    using Addr = uint64_t;
    using RegId = int8_t;

    SiameseProfiler();

    ~SiameseProfiler()
    {}

    void init();

    void initPhase();

    void profile(DecodeExecState &s);

  private:
    uint64_t intervalSize;

    uint64_t intervalCount;

    uint64_t intervalDrift;

    OutputStream *outStream;

    unsigned footprintPeriodSize;

    struct ControlflowInfo
    {
        using BranchInfo = std::unordered_map<Addr, unsigned>;
        std::unordered_map<Addr, BranchInfo> branchOutcomeTree;

        void add(Addr pc, Addr target);
        void periodClear();

        float computeEntropy (BranchInfo &info);

        unsigned TopN{3};
        struct BranchFeatures {
            unsigned totalCount{};
            unsigned nonTrivialCount{};
//            float outcomeRatio;
            float nonTrivialOutcomeRatio;
            float topNOutcomeRatio;
            float topNOutcomeProportion;
            float overallEntropy{};
            float topNEntropy{};
        };

        struct BranchComparator
        {
            bool operator()(
                BranchInfo * const & a, BranchInfo * const & b)
            {
                return a->size() > b->size();
            }
        };

        SizedRanker<BranchInfo*, BranchComparator> ranker;
        BranchFeatures computeBranchFeatures();

        std::list<BranchFeatures> featureRecords;

        void push(const BranchFeatures& feature);

    } controlflowInfo;

    struct MemAccInfo
    {
        unsigned LastN{32};
        std::deque<Addr> lastNMemAcc;
        std::unordered_set<Addr> physAddrSet;
        void add(Addr addr);
        unsigned count();
        void periodClear();

        unsigned DiversTableSize{1091};
        std::vector<unsigned> table;

        unsigned TopN{5};
        struct Diversity {
            float topNProportion;
            float nonZeroProportion;
        };
        Diversity computeDistanceDiversity();

        struct MemFeatures {
            Diversity diversity;
            unsigned fpSize;
            MemFeatures(const Diversity &divers, unsigned fp_size) {
                diversity = divers;
                fpSize = fp_size;
            }
        };

        std::list<MemFeatures> features;
        void push(const Diversity &diversity, unsigned fp_size);

    } memAccInfo;

    struct DataflowInfo
    {
        unsigned maxPathLength;
        std::unordered_map<Addr, unsigned> paths;
        void addStore(SiameseProfiler::Addr addr, Operand *src1,
                      Operand *src2);

        Addr hash(RegId reg, bool is_float);

        void addLoad(Addr addr, Operand *src1, Operand *dest);

        std::pair<bool, unsigned> findPathLen(Addr addr);
        std::pair<bool, unsigned> findPathLen(bool is_float, Operand *reg);

        void addReg(Operand *dest, Operand *src1, Operand *src2, RegId src3,
                    bool is_fma);

        unsigned getCriticalPath();
        std::list<uint64_t> critPathHistory;
        void push(uint64_t len);
        void periodClear();

    } dataflowInfo;

    struct PhaseInfo
    {
    };

    void dumpMemAccInfo();

    void smallWindowDumpMemAccInfo();

    void dumpDataflowInfo();

    void smallWindowDumpDataflowInfo();

    void dumpControlflowInfo();

    // dump header for csv
    void dumpHeader();
};

extern SiameseProfiler profiler;

#endif //__SIAMESE_PROFILER__
