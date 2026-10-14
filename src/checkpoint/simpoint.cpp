/*
 * Copyright (c) 2012-2014 ARM Limited
 * All rights reserved.
 *
 * The license below extends only to copyright in the software and shall
 * not be construed as granting a license to any other intellectual
 * property including but not limited to intellectual property relating
 * to a hardware implementation of the functionality of the software
 * licensed hereunder.  You may use the software subject to the license
 * terms below provided that you ensure that this notice is replicated
 * unmodified and in its entirety in all distributions of the software,
 * modified or unmodified, in source code or in binary form.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Authors: Dam Sunwoo
 *          Curtis Dunham
 */

#include "checkpoint/path_manager.h"
#include <cassert>
// #include <debug.h>
#include <algorithm>
#include <charconv>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <vector>

#include <checkpoint/simpoint.h>
#include <profiling/profiling_control.h>
#include <profiling/lf.h>

namespace SimPointNS
{

extern "C" {
#include <debug.h>
extern uint64_t record_row_number;
extern FILE *log_fp;
extern char *log_filebuf;
extern void log_buffer_flush();
extern void log_file_flush();
extern bool enable_fast_log;
extern bool enable_small_log;
}

SimPoint::SimPoint()
    : intervalCount(0),
      intervalDrift(0),
      simpointStream(nullptr),
      currentBBV(0, 0),
      currentBBVInstCount(0) {
}

SimPoint::~SimPoint() {
  if (simpointStream)
    NEMUNS::simout.close(simpointStream);
}

void
SimPoint::init() {
  if (profiling_state == SimpointProfiling) {
    bbMap.reserve(bbLookupCacheSize);
    pathManager.setSimpointProfilingOutputDir();
    assert(checkpoint_interval);
    intervalSize = checkpoint_interval;
#ifdef CONFIG_LF_PROFILING
    if (lf_config.requested) {
      // The leader never owns the BBV stream: its destructor would truncate
      // the merged file. Followers write fragments that the leader merges.
      Log("Leader-follower simpoint profiling with interval %lu", intervalSize);
      return;
    }
#endif
    Log("Doing simpoint profiling with interval %lu", intervalSize);
    auto path = pathManager.getOutputPath() + "/simpoint_bbv.gz";

    using NEMUNS::simout;
    simpointStream = simout.create(path, false);

    if (!simpointStream)
      xpanic("unable to open SimPoint profile_file %s\n", path.c_str());
  }
}

void
SimPoint::profile_with_abs_icount(Addr pc, bool is_control, bool is_last_uop, uint64_t abs_icount) {
  unsigned exec_count = abs_icount - lastICount;
  Logsp("PC: 0x%lx , icount: %lu, control: %i", pc, abs_icount, is_control);
  Logsp("is_control: %i, is_last_uop: %i, exec_count: %u", is_control, is_last_uop, exec_count);
  profile(pc, is_control, is_last_uop, exec_count);
  lastICount = abs_icount;
}

void
SimPoint::profile_basic_block_with_abs_icount(Addr control_pc, Addr next_pc, uint64_t abs_icount) {
  unsigned exec_count = abs_icount - lastICount;
  profile(control_pc, true, true, exec_count);
  currentBBV.first = next_pc;
  lastICount = abs_icount;
}

void
formatInterval(std::string &line, const std::vector<std::pair<uint64_t, uint64_t> > &counts) {
  char num[24];
  line.push_back('T');
  for (const auto &c : counts) {
    line.push_back(':');
    line.append(num, std::to_chars(num, num + sizeof(num), c.first).ptr);
    line.push_back(':');
    line.append(num, std::to_chars(num, num + sizeof(num), c.second).ptr);
    line.push_back(' ');
  }
  line.push_back('\n');
}

void
SimPoint::collectCounts(std::vector<std::pair<uint64_t, uint64_t> > &counts) {
  for (auto map_itr = bbMap.begin(); map_itr != bbMap.end(); ++map_itr) {
    BBInfo &info = map_itr->second;
    if (info.count != 0) {
      counts.push_back(std::make_pair(info.id, info.count));
      info.count = 0;
    }
  }
  std::sort(counts.begin(), counts.end());
}

#ifdef CONFIG_LF_PROFILING
void
SimPoint::beginSlice(bool pristine, uint64_t profiledICount, Addr nextPC) {
  bbMap.clear();
  bbLookupCache.fill(BBLookupCacheEntry{});
  segments.clear();
  sliceMode = true;
  sliceIntervalEnds = 0;
  driftViolated = false;
  currentBBVInstCount = 0;
  intervalDrift = 0;
  if (pristine) {
    lastICount = 0;
    currentBBV = BasicBlockRange(0, 0);
    intervalCount = 0;
  } else {
    lastICount = profiledICount;
    currentBBV.first = nextPC;
    // Only intervalCount + intervalDrift affects the output.
    intervalCount = profiledICount % intervalSize;
  }
}

bool
SimPoint::endSlice(const char *path, lf_fragment_header_t *hdr, std::string &err) {
  std::vector<std::pair<uint64_t, uint64_t> > tail;
  collectCounts(tail);
  segments.push_back(Segment{false, std::move(tail)});

  // Ids are assigned in first-publication order, starting from 1 in a slice.
  std::vector<BasicBlockRange> firstSeen(bbMap.size());
  for (const auto &e : bbMap) firstSeen[e.second.id - 1] = e.first;

  const uint64_t start = hdr->start_profiled, end = hdr->end_profiled;
  hdr->interval_size = intervalSize;
  hdr->first_interval = start / intervalSize;
  hdr->nr_blocks = firstSeen.size();
  hdr->nr_segments = segments.size();
  if (driftViolated) {
    err = "a publication spanned more than one interval";
    return false;
  }
  if (!hdr->run_ended) {
    if (lastICount != end) {
      err = "last publication at " + std::to_string(lastICount) + ", expected " + std::to_string(end);
      return false;
    }
    if ((intervalCount + intervalDrift) % intervalSize != end % intervalSize ||
        intervalCount + intervalDrift >= intervalSize) {
      err = "interval residual " + std::to_string(intervalCount + intervalDrift) + ", expected " +
            std::to_string(end % intervalSize);
      return false;
    }
    if (sliceIntervalEnds != end / intervalSize - start / intervalSize) {
      err = std::to_string(sliceIntervalEnds) + " interval ends, expected " +
            std::to_string(end / intervalSize - start / intervalSize);
      return false;
    }
  }

  std::string tmp = std::string(path) + ".tmp";
  FILE *fp = fopen(tmp.c_str(), "wb");
  if (fp == NULL) {
    err = "cannot create " + tmp + ": " + strerror(errno);
    return false;
  }
  bool ok = fwrite(hdr, sizeof(*hdr), 1, fp) == 1;
  for (const auto &bb : firstSeen) {
    uint64_t v[2] = {bb.first, bb.second};
    ok = ok && fwrite(v, sizeof(v), 1, fp) == 1;
  }
  for (const auto &seg : segments) {
    uint64_t v[2] = {seg.closed, seg.counts.size()};
    ok = ok && fwrite(v, sizeof(v), 1, fp) == 1;
    for (const auto &c : seg.counts) {
      uint64_t e[2] = {c.first, c.second};
      ok = ok && fwrite(e, sizeof(e), 1, fp) == 1;
    }
  }
  ok = (fclose(fp) == 0) && ok;
  if (!ok || rename(tmp.c_str(), path) != 0) {
    err = "cannot write " + std::string(path) + ": " + strerror(errno);
    unlink(tmp.c_str());
    return false;
  }
  return true;
}
#endif

void
SimPoint::profile(Addr pc, bool is_control, bool is_last_uop, unsigned instr_count) {

  if (!is_last_uop)
    return;

  intervalCount += instr_count;
  currentBBVInstCount += instr_count;

  if (!currentBBVInstCount) {
    Logsp("Set BB start: 0x%lx", pc);
    currentBBV.first = pc;
  }

  Logsp("intervalCount: %lu, currentBBVInstCount: %lu", intervalCount, currentBBVInstCount);

  // If inst is control inst, assume end of basic block.
  if (is_control) {
    currentBBV.second = pc;

    const size_t cache_idx = std::hash<BasicBlockRange>()(currentBBV) &
                             (bbLookupCacheSize - 1);
    BBLookupCacheEntry &cache_entry = bbLookupCache[cache_idx];
    BBInfo *info = cache_entry.info;
    if (info != nullptr && cache_entry.range == currentBBV) {
      info->count += currentBBVInstCount;
    } else {
      auto map_itr = bbMap.find(currentBBV);
      Logsp("Finding BB 0x%lx -> 0x%lx", currentBBV.first, currentBBV.second);
      if (map_itr == bbMap.end()) {
        BBInfo new_info;
        new_info.id = bbMap.size() + 1;
        new_info.insts = currentBBVInstCount;
        new_info.count = currentBBVInstCount;
        map_itr = bbMap.insert(::std::make_pair(currentBBV, new_info)).first;
      } else {
        map_itr->second.count += currentBBVInstCount;
      }
      cache_entry.range = currentBBV;
      cache_entry.info = &map_itr->second;
    }
    currentBBVInstCount = 0;

    // Reached end of interval if the sum of the current inst count
    // (intervalCount) and the excessive inst count from the previous
    // interval (intervalDrift) is greater than/equal to the interval size.
    if (intervalCount + intervalDrift >= intervalSize) {
#ifdef CONFIG_PROFILING_DETAIL_MODEL
      *simpointStream->stream() << "T";
      for (auto map_itr = bbMap.begin(); map_itr != bbMap.end(); ++map_itr) {
        *simpointStream->stream() << ":" << map_itr->first.first << ":" << map_itr->first.second << ":"
          << map_itr->second.id << ":" << map_itr->second.count << " ";
      }
      *simpointStream->stream() << "\n";
#else
      // summarize interval and display BBV info
      std::vector<std::pair<uint64_t, uint64_t> > counts;
      collectCounts(counts);

#ifdef CONFIG_LF_PROFILING
      if (sliceMode) {
        segments.push_back(Segment{true, std::move(counts)});
        sliceIntervalEnds++;
      } else
#endif
      {
        // Print output BBV info
        std::string line;
        formatInterval(line, counts);
        simpointStream->stream()->write(line.data(), line.size());
      }
#endif
      Logsp("Simpoint profilied %lu instrs", intervalCount);

      intervalDrift = (intervalCount + intervalDrift) - intervalSize;
      intervalCount = 0;
#ifdef CONFIG_LF_PROFILING
      // Slice state is derived assuming at most one interval ends per
      // publication, which keeps the drift below one interval.
      if (sliceMode && intervalDrift >= intervalSize) driftViolated = true;
#endif
    }
  }
}

}

SimPointNS::SimPoint simpoit_obj;

extern "C" {

void simpoint_init() {
  simpoit_obj.init();
}

#ifndef CONFIG_SHARE
void simpoint_profiling(uint64_t pc, bool is_control, uint64_t abs_instr_count) {
  simpoit_obj.profile_with_abs_icount(pc, is_control, true, abs_instr_count);
}

__attribute__((noinline))
void simpoint_profiling_bb(uint64_t control_pc, uint64_t next_pc, uint64_t abs_instr_count) {
  simpoit_obj.profile_basic_block_with_abs_icount(control_pc, next_pc, abs_instr_count);
}
#endif

#ifdef CONFIG_LF_PROFILING
void simpoint_lf_begin_slice(bool pristine, uint64_t profiled_icount, uint64_t next_pc) {
  simpoit_obj.beginSlice(pristine, profiled_icount, next_pc);
}

// Returns 0 on success; otherwise writes the reason into err.
int simpoint_lf_end_slice(const char *path, lf_fragment_header_t *hdr, char *err, size_t err_len) {
  std::string reason;
  if (simpoit_obj.endSlice(path, hdr, reason)) return 0;
  snprintf(err, err_len, "%s", reason.c_str());
  return -1;
}

int simpoint_lf_output_dir(char *buf, size_t len) {
  std::string dir = pathManager.getOutputPath();
  while (dir.size() > 1 && dir.back() == '/') dir.pop_back();
  if (dir.size() + 1 > len) return -1;
  memcpy(buf, dir.c_str(), dir.size() + 1);
  return 0;
}
#endif
}
