#ifndef __PROFILING_PPM_H__
#define __PROFILING_PPM_H__

#include <cinttypes>
#include <string>
#include <vector>
#include <unordered_map>
#include <boost/dynamic_bitset.hpp>

namespace PPMNS {

struct PredNode {
    uint8_t valid;
    uint8_t takenBias;
    uint8_t biasCount;
    uint8_t isLeaf;
};

struct TableKey {
    uint64_t pc;
    unsigned histLen;
    std::string hist;
    bool operator==(const TableKey &other) const {
        return pc == other.pc && histLen == other.histLen && hist == other.hist;
    }
};
} //namespace PPMNS

namespace std {

template <>
struct hash<PPMNS::TableKey> {
    std::size_t operator()(PPMNS::TableKey const &k) const {
        using std::hash;
        using std::string;

        return hash<string>()(k.hist) ^ (hash<uint64_t>()(k.pc >> 1));
    }
};

} // std

namespace PPMNS {
class PPM {
    uint8_t counterLimit{2};
    unsigned histLen{64};
    unsigned directLookupLen{10};

    std::vector<boost::dynamic_bitset<>> masks;

    using PredLUT = std::vector<std::vector<PredNode>>;
    std::unordered_map<uint64_t, PredLUT> directLuts;

    std::unordered_map<TableKey, PredNode> table;

  public:
    uint64_t correct{};
    uint64_t mispred{};

    PPM();

    void update(uint64_t pc, boost::dynamic_bitset<> &hist, bool taken, bool pred_taken);

    bool lookup(uint64_t pc, boost::dynamic_bitset<> &hist);
};

} // namespace PPMNS



#endif //__PROFILING_PPM_H__