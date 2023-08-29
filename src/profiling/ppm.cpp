#include <profiling/ppm.h>
#include <cassert>
#include <cmath>
#include <debug.h>


namespace PPMNS {

using boost::dynamic_bitset;

PPM::PPM(){
    for (unsigned len = 0; len <= histLen; len++) {
        masks.push_back(dynamic_bitset<>(histLen, (1 << len) - 1));
    }
}

void PPM::update(uint64_t pc, dynamic_bitset<> &hist, bool taken, bool pred_taken) {
    correct += taken == pred_taken;
    mispred += taken != pred_taken;
    Logbeta("Mispredicted: %i, should taken: %i, pred taken: %i", taken != pred_taken, taken, pred_taken);
    bool direct_table_wrong = false;
    bool longest_direct_table_split = false;
    for (unsigned len = 0; len <= hist.size(); len++) {
        if (len <= directLookupLen) {
            uint64_t h = (hist & masks[len]).to_ulong();
            auto &node = directLuts[pc][len][h];
            Logbeta("Updating luts[%u] for pc 0x%010lx with history 0x%010lx",
            len, pc, h);
            if (node.valid) {
                if (taken == node.takenBias) {
                    node.biasCount = std::min((unsigned) node.biasCount + 1, (unsigned) counterLimit);
                    Logbeta("Inc confidence -> %u", node.biasCount);
                } else { // disagree
                    if (node.biasCount == 0) {
                        node.takenBias = taken;
                        Logbeta("Flip bias");
                    } else {
                        node.biasCount -= 1;
                        Logbeta("Dec confidence -> %u", node.biasCount);
                    }
                    if (len == directLookupLen) {
                        Logbeta("Disagree with longest direct table");
                        direct_table_wrong = true;
                        node.isLeaf = false;
                    }
                }
                if (len == directLookupLen && !node.isLeaf) {
                    longest_direct_table_split = true;
                }
            } else {
                Logbeta("Create new direct entry with taken: %i", taken);
                node.valid = true;
                node.takenBias = taken;
                node.biasCount = 1;
                node.isLeaf = true;
            }
        } else if (longest_direct_table_split || direct_table_wrong){
            std::string h;
            boost::to_string(hist & masks[len], h);
            // Logbeta("Update disagree entry with taken: %i, hist: %s", taken, h.c_str());
            auto key = TableKey{pc, len, h};
            const auto entry = table.find(key);
            if (entry == table.end()) { // new entry
                auto it = table.insert({key, PredNode()});
                auto &node = it.first->second;
                node.valid = true;
                node.isLeaf = true;
                node.takenBias = taken;
                node.biasCount = 1;
                Logbeta("Insert extended entry with len: %u, hist: %s, taken: %i",
                    len, h.c_str(), node.takenBias);
                break;
            } else {
                auto &node = entry->second;
                assert(node.valid);
                Logbeta("Update found extended entry with len: %u, hist: %s, taken: %i, conf: %u",
                    len, h.c_str(), node.takenBias, node.biasCount);
                if (node.takenBias != taken) {
                    if (node.biasCount >= 1) { // exists and disagrees
                        Logbeta("Dec confidence and split");
                        node.biasCount -= 1;
                        node.isLeaf = false;
                    } else { // exists and flipable
                        Logbeta("Flip bias");
                        node.takenBias = taken;
                        break;
                    }
                } else { // exists and agrees
                    Logbeta("Inc confidence");
                    node.biasCount = std::min((unsigned) node.biasCount + 1,
                        (unsigned) counterLimit);
                    if (node.isLeaf) {
                        break;
                    }
                }
            }
        }
    }
}

bool PPM::lookup(uint64_t pc, dynamic_bitset<> &hist) {
    Logbeta("Lookup pc 0x%010lx", pc);
    const auto direct_lut_it = directLuts.find(pc);
    if (direct_lut_it == directLuts.end()) {
        directLuts.insert({pc, PredLUT(directLookupLen + 1)});
        // directLuts[pc] = PredLUT(directLookupLen);
        for (unsigned i = 0; i <= directLookupLen; i++) {
            directLuts[pc][i].resize(powl(2, i));
        }
        auto &root = directLuts[pc][0][0];
        root.valid = true;
        root.isLeaf = true;
        root.biasCount = 0;
        root.takenBias = 0;
    }
    const auto &luts = directLuts[pc];
    assert(hist.size() <= histLen);
    bool valid_found = false;
    for (int len = directLookupLen; len >= 0; len--) {
        // Logbeta("luts size: %lu, mask size: %lu, len: %u",
        // luts.size(), masks.size(), len);
        const auto &lut = luts[len];
        uint64_t h = (hist & masks[len]).to_ulong();
        Logbeta("luts size: %lu, mask size: %lu, len: %i, h: %016lx, lut size: %lu",
        luts.size(), masks.size(), len, h, lut.size());
        const auto &node = lut.at(h);
        Logbeta("Reach x");
        if (node.valid) {
            valid_found = true;
            if (len == (int) directLookupLen && !node.isLeaf) {
                break;
            } else {
                Logbeta("Found valid leaf entry at len %i", len);
                return node.takenBias;
            }
        }
        Logbeta("Reach z");
    }
    assert(valid_found);
    Logbeta("Last direct table indicating split, to look up from disagree table");
    
    // look up from disagree table
    bool last_valid_taken;
    for (unsigned len = directLookupLen + 1; len <= histLen; len++) {
        std::string h;
        boost::to_string(hist & masks[len], h);
        auto key = TableKey{pc, len, h};
        const auto ptr = table.find(key);
        if (ptr != table.end()) {
            const auto &node = ptr->second;
            assert(node.valid);
            if (node.isLeaf) {
                Logbeta("Found extended entry with len: %u, hist: %s, taken: %i",
                    len, h.c_str(), node.takenBias);
                return node.takenBias;
            } else {
                Logbeta("Skip non-leaf extended entry with len: %u, hist: %s, taken: %i",
                    len, h.c_str(), node.takenBias);
            }
            last_valid_taken = node.takenBias;
        }
    }
    Logbeta("Returning last valid entry, taken: %i", last_valid_taken);
    return last_valid_taken;
}

}