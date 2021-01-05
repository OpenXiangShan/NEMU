//
// Created by zyy on 2020/12/22.
//
#include <algorithm>
#include <cmath>
#include <iostream>
#include <numeric>
#include <vector>

#include <base/sized_ranker.h>
#include <checkpoint/path_manager.h>
#include <checkpoint/profiler.h>
#include <monitor/monitor.h>

void
SiameseProfiler::profile(
    DecodeExecState &s, uint64_t inst_count)
{
  if (s.is_load || s.is_store) {
    memAccInfo.add(s.src1.val + s.src2.imm);
  }

  if (s.is_store) {
//      Log("is store");
    dataflowInfo.addStore(s.src1.val + s.src2.imm, &s.src1, &s.dest);
  } else if (s.is_load) {
//      Log("is load");
    dataflowInfo.addLoad(s.src1.val + s.src2.imm, &s.src1, &s.dest);
  } else {
//      Log("is reg");
    dataflowInfo.addReg(&s.dest, &s.src1, &s.src2, s.isa.instr.fp.funct5, s.is_fma);
  }

  if (s.is_control) {
    controlflowInfo.add(s.seq_pc, s.jmp_pc);
  }
  intervalCount++;
  if (intervalCount + intervalDrift >= intervalSize) {
    *outStream->stream() << inst_count;
    dumpMemAccInfo();
    dumpDataflowInfo();
    dumpControlflowInfo();

    intervalDrift = (intervalCount + intervalDrift) - intervalSize;
//      Log("Dumping with interval count %lu, drift: %lu\n", intervalCount, intervalDrift);
    intervalCount = 0;
  }
}

void SiameseProfiler::dumpHeader()
{

  *outStream->stream()
      << "inst.count"
      << "," << "divers.topNProportion"
      << "," << "divers.nonZeroProportion"
      << "," << "footprint"

      << "," << "dataflowInfo.CriticalPathLen"

      << "," << "totalBranchCount"
      << "," << "nonTrivialBranchCount"
      << "," << "nonTrivialOutcomeRatio"
      << "," << "topNOutcomeRatio"
      << "," << "topNOutcomeProportionInTotal"
      << "," << "overallEntropy"
      << "," << "topNEntropy"
      << "\n";
}

void SiameseProfiler::dumpMemAccInfo()
{
  auto divers = memAccInfo.computeDistanceDiversity();
  *outStream->stream()
      << "," << divers.topNProportion
      << "," << divers.nonZeroProportion
      << "," << memAccInfo.count();
  memAccInfo.periodClear();
}

void SiameseProfiler::dumpDataflowInfo()
{
  *outStream->stream()
      << "," << dataflowInfo.getCriticalPath();
  dataflowInfo.periodClear();
}

void SiameseProfiler::dumpControlflowInfo()
{
  // TODO: dump branchFeatures
  auto features = controlflowInfo.computeBranchFeatures();

  *outStream->stream()
      << "," << features.totalCount
      << "," << features.nonTrivialCount
      //        << "," << features.outcomeRatio
      << "," << features.nonTrivialOutcomeRatio
      << "," << features.topNOutcomeRatio
      << "," << features.topNOutcomeProportion
      << "," << features.overallEntropy
      << "," << features.topNEntropy
      << "\n";
  controlflowInfo.periodClear();
//    controlflowInfo.push(controlflowInfo.computeBranchFeatures());
}


void SiameseProfiler::smallWindowDumpMemAccInfo()
{
//    memAccInfo.push(memAccInfo.computeDistanceDiversity(), memAccInfo.count());
//    memAccInfo.periodClear();
}

void SiameseProfiler::smallWindowDumpDataflowInfo()
{
//    dataflowInfo.push(dataflowInfo.getCriticalPath());
//    dataflowInfo.periodClear();
}

void SiameseProfiler::init()
{
  memAccInfo.table.resize(memAccInfo.DiversTableSize);
  if (profiling_state == BetapointProfiling) {
    assert(profiling_interval);
    intervalSize = profiling_interval;
    Log("Doing betapoint profiling with interval %lu", intervalSize);
    auto path = pathManager.getOutputPath() + "/betapoint.csv.gz";
    outStream = simout.create(path, false);
    if (!outStream)
      xpanic("unable to open Betapoint profile_file %s\n", path.c_str());
  }
  controlflowInfo.ranker.set_size(controlflowInfo.TopN);
  dumpHeader();
}

SiameseProfiler::SiameseProfiler()
{
}

void SiameseProfiler::MemAccInfo::periodClear()
{
  physAddrSet.clear();
  std::fill(table.begin(), table.end(), 0);
}

void SiameseProfiler::MemAccInfo::add(SiameseProfiler::Addr addr)
{
  addr = addr >> 6; // assumed cache block
  physAddrSet.insert(addr);

  for (auto old_addr: lastNMemAcc) {
    auto dist = addr - old_addr;
    auto index = dist % DiversTableSize;
    table[index]++;
  }

  lastNMemAcc.push_back(addr);
  if (lastNMemAcc.size() > LastN) {
    lastNMemAcc.pop_front();
  }
}

unsigned SiameseProfiler::MemAccInfo::count()
{
  return physAddrSet.size();
}

SiameseProfiler::MemAccInfo::Diversity SiameseProfiler::MemAccInfo::computeDistanceDiversity()
{
  std::sort(table.begin(), table.end(), std::greater<>());
  unsigned topn_sum = 0;
  for (unsigned i = 0; i < TopN; i++) {
    topn_sum += table[i];
  }
  unsigned total_non_zero = 0, sum = 0;
  for (auto u : table) {
    if (u <= 0) {
      break;
    }
    total_non_zero++;
    sum += u;
  }

  Diversity diversity;
  diversity.nonZeroProportion = ((float) total_non_zero) / (float) DiversTableSize;
  diversity.topNProportion = ((float) topn_sum) / (float) sum;
  return diversity;
}

void SiameseProfiler::MemAccInfo::push(
    const SiameseProfiler::MemAccInfo::Diversity &diversity, unsigned int fp_size)
{
  features.emplace_back(diversity, fp_size);
}

void
SiameseProfiler::DataflowInfo::addStore(
    SiameseProfiler::Addr addr, Operand *src1,
    Operand *src2)
{
  unsigned max_len = 0;

  auto[valid, len] = findPathLen(src1->is_fp, src1);
  if (valid)
    max_len = len;

  std::tie(valid, len) = findPathLen(src2->is_fp, src2);
  if (valid && len > max_len)
    max_len = len;

  paths[addr] = max_len + 1;
}

SiameseProfiler::Addr SiameseProfiler::DataflowInfo::hash(
    SiameseProfiler::RegId reg, bool is_float)
{
  return 0x300000000L + reg + is_float * 1024;
}

void SiameseProfiler::DataflowInfo::addLoad(
    Addr addr, Operand *src1,
    Operand *dest)
{
  unsigned max_len = 0;

  auto[valid, len] = findPathLen(src1->is_fp, src1);
  if (valid)
    max_len = len;

  std::tie(valid, len) = findPathLen(addr);
  if (valid && len > max_len)
    max_len = len;

  paths[hash(dest->reg, dest->is_fp)] = max_len + 1;
}

std::pair<bool, unsigned> SiameseProfiler::DataflowInfo::findPathLen(SiameseProfiler::Addr addr)
{
  bool found = false;
  unsigned len = 0;

  auto it = paths.find(addr);
  if (it != paths.end()) {
    found = true;
    len = it->second;
//        Log("Found 0x%x with path len: %u", addr, it->second);
  }
  return std::make_pair(found, len);
}

std::pair<bool, unsigned> SiameseProfiler::DataflowInfo::findPathLen(
    bool is_float, Operand *reg)
{
  if (reg->type != OP_TYPE_REG || (!is_float && reg->reg == 0)) {
    return std::make_pair(true, 0);
  }
  return findPathLen(hash(reg->reg, is_float));
}

void SiameseProfiler::DataflowInfo::addReg(
    Operand *dest,
    Operand *src1, Operand *src2, RegId src3,
    bool is_fma)
{
//  Log("%i %i %i", dest->type == OP_TYPE_REG, dest->is_fp, dest->reg);
  if (dest->type != OP_TYPE_REG || (!dest->is_fp && (dest->reg == 0))) { // zero reg
    return;
  }
  unsigned max_len = 0;

  auto[valid, len] = findPathLen(src1->is_fp, src1);
  if (valid)
    max_len = len;

  std::tie(valid, len) = findPathLen(src2->is_fp, src2);
  if (valid && len > max_len)
    max_len = len;

  if (is_fma) {
    std::tie(valid, len) = findPathLen(hash(src3, true));
    if (valid && len > max_len)
      max_len = len;
  }

//    Log("Setting 0x%lx to %u", hash(dest->reg, dest->is_fp), max_len + 1);
  paths[hash(dest->reg, dest->is_fp)] = max_len + 1;
}

unsigned SiameseProfiler::DataflowInfo::getCriticalPath()
{
  unsigned m = 0;
  for (auto p: paths) {
    if (p.second > m) {
      m = p.second;
    }
  }
  return m;
}

void SiameseProfiler::DataflowInfo::push(uint64_t len)
{
  critPathHistory.push_back(len);
}

void SiameseProfiler::DataflowInfo::periodClear()
{
  paths.clear();
}

void SiameseProfiler::ControlflowInfo::periodClear()
{
  branchOutcomeTree.clear();
}

void SiameseProfiler::ControlflowInfo::add(SiameseProfiler::Addr pc, SiameseProfiler::Addr target)
{
  branchOutcomeTree[pc][target];
  branchOutcomeTree[pc][target]++;
}

SiameseProfiler::ControlflowInfo::BranchFeatures
SiameseProfiler::ControlflowInfo::computeBranchFeatures()
{
  // TODO: complete it
  BranchFeatures brFeatures;

  // filter trivial jumps
  for (auto &branch: branchOutcomeTree) {
    if (!branch.second.empty()) {
      brFeatures.totalCount++;
    }
    if (branch.second.size() > 1) {
      brFeatures.nonTrivialCount++;
    }
    ranker.possiblyAdd(&(branch.second));
    brFeatures.overallEntropy += computeEntropy(branch.second);
  }
//    brFeatures.outcomeRatio = (float) brFeatures.totalCount / branchOutcomeTree.size();
  brFeatures.nonTrivialOutcomeRatio = (float) brFeatures.nonTrivialCount / branchOutcomeTree.size();
  unsigned topNOutcomeCount = 0;
  auto &pq = ranker.get();
  while (!pq.empty()) {
    auto branch = pq.top();
    brFeatures.topNEntropy += computeEntropy(*branch);
    topNOutcomeCount += branch->size();
    pq.pop();
  }
  brFeatures.topNOutcomeRatio = (float) topNOutcomeCount / (float) TopN;
  brFeatures.topNOutcomeProportion = (float) topNOutcomeCount / (float) brFeatures.totalCount;

  return brFeatures;
}

float
SiameseProfiler::ControlflowInfo::computeEntropy(
    SiameseProfiler::ControlflowInfo::BranchInfo &info)
{
  unsigned sum = 0;
  for (auto &target: info) {
    sum += target.second;
  }

  double entropy = 0.0;
  for (auto &target: info) {
    double p = (double) target.second / sum;
    entropy -= p * log2((double) p);
  }
  return (float) entropy;
}

void
SiameseProfiler::ControlflowInfo::push(
    const SiameseProfiler::ControlflowInfo::BranchFeatures &feature)
{
  featureRecords.push_back(feature);
}

SiameseProfiler profiler;

bool xpoint_profiling_started;

void init_profiler()
{
  profiler.init();
  xpoint_profiling_started = false;
}
