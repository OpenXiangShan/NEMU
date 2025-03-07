#include "common.h"
#include "profiling/profiling_control.h"
#include <cassert>
#include <cstddef>
#include <functional>
#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <utility>

namespace SematicPointNS{

typedef std::pair<vaddr_t, vaddr_t> BasicBlockRange;

struct BasicBlockRangeHash {
  size_t operator()(const BasicBlockRange& p) const {
    return std::hash<vaddr_t>()(p.first + p.second);
  }
};

class SematicPoint{
  public:
    SematicPoint();

    virtual void init(const char* sematic_point_path);


    void before_profile(vaddr_t pc, bool is_control, uint64_t abs_icount);
    void profile(vaddr_t pc, bool is_control, uint64_t instr_count);
    bool check_counter();

    bool enable{false};

  private:
    ::std::unordered_map<BasicBlockRange, uint64_t, BasicBlockRangeHash> sematic_info;
    ::std::unordered_map<BasicBlockRange, uint64_t, BasicBlockRangeHash> counter;

    BasicBlockRange currentBBV;
    uint64_t lastICount{0};
};

SematicPoint::SematicPoint()
  : lastICount(0) {}

void SematicPoint::init(const char* sematic_point_path){
  if (sematic_point_path == NULL) {
    return;
  }

  std::ifstream file(sematic_point_path);
  if (!file) {
    std::cerr << "Failed to open file\n";
    return;
  }

  std::string line;
  while (std::getline(file, line)) {
    if (line.empty()) {
      break;
    }
    size_t comma1 = line.find(",");
    if (comma1 == std::string::npos) {
      std::cerr << "Invalid format: " << line << std::endl;
      break;
    }

    size_t comma2 = line.find(",", comma1 + 1);
    if (comma2 == std::string::npos) {
      std::cerr << "Invalid format: " << line << std::endl;
    }

    std::string first_pc_str = line.substr(0, comma1);
    std::string second_pc_str = line.substr(comma1 + 1, comma2 - comma1 - 1);
    std::string val_str = line.substr(comma2 + 1);

    try{
      vaddr_t first_pc = std::stoull(first_pc_str, nullptr, 16);
      vaddr_t second_pc = std::stoull(second_pc_str, nullptr, 16);

      uint64_t value = std::stoull(val_str, nullptr, 10);
      BasicBlockRange temp = {first_pc, second_pc};

      sematic_info[temp] = value;
      counter[temp] = 0;
    } catch (const std::exception& e){
      std::cerr << "Conversion error: " << e.what() << " in line: " << line << std::endl;
    }
  }

  if (!sematic_info.empty()) {
    std::cout << "\nSample entries (max 5):\n";
    size_t count = 0;
    for (const auto& [range, value] : sematic_info) {
      std::cout << std::hex << "[0x" << range.first
                << ", 0x" << range.second << "] => "
                << std::dec << value << "\n";
      if (++count >= 5) break;
    }
  }
  file.close();

  enable = true;
  checkpoint_state = SematicCheckpointing;
  return;
}

void SematicPoint::profile(vaddr_t pc, bool is_control, uint64_t instr_count){
  if (!instr_count) {
    currentBBV.first = pc;
  }

  if (is_control) {
    currentBBV.second = pc;

//    std::cout << "bbv info:" << std::hex << currentBBV.first << "," << currentBBV.second << std::endl;
    auto map_iter = counter.find(currentBBV);
    if (map_iter == counter.end()) {
      return;
    }else{
      map_iter->second = map_iter->second + 1;
    }

  }
}

void SematicPoint::before_profile(vaddr_t pc, bool is_control, uint64_t abs_icount) {
  profile(pc, is_control, abs_icount - lastICount);
  lastICount = abs_icount;
}

bool SematicPoint::check_counter() {
  for (const auto& [bb_range, value] : sematic_info) {
    auto it = counter.find(bb_range);

    if (it == counter.end()) {
      assert(0);
    }

    if (it->second < value) {
      return false;
    }
  }
  return true;
}

};


SematicPointNS::SematicPoint sematic_point;

extern "C" {
  void sematic_point_init(const char* sematic_point_path) {
    sematic_point.init(sematic_point_path);
  }

  void sematic_point_profile(vaddr_t pc, bool is_control, uint64_t instr_count) {
    sematic_point.before_profile(pc, is_control, instr_count);
  }

  bool check_sematic_point() {
    return sematic_point.check_counter();
  }

  bool enable_sematic_point_cpt() {
    return sematic_point.enable;
  }
}
