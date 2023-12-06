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
//#include <debug.h>
#include <vector>
#include <algorithm>
#include <iostream>

#include <checkpoint/simpoint.h>
#include <profiling/profiling_control.h>

namespace SimPointNS
{

extern "C" {
#include <debug.h>
extern bool log_enable();
extern void log_flush();
extern char *log_filebuf;
extern uint64_t record_row_number;
extern FILE *log_fp;
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
    pathManager.setSimpointProfilingOutputDir();
    assert(checkpoint_interval);
    intervalSize = checkpoint_interval;
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

    auto map_itr = bbMap.find(currentBBV);
    Logsp("Finding BB 0x%lx -> 0x%lx", currentBBV.first, currentBBV.second);
    if (map_itr == bbMap.end()) {
      // If a new (previously unseen) basic block is found,
      // add a new unique id, record num of insts and insert into bbMap.
      BBInfo info;
      info.id = bbMap.size() + 1;
      info.insts = currentBBVInstCount;
      info.count = currentBBVInstCount;
      bbMap.insert(::std::make_pair(currentBBV, info));
    } else {
      // If basic block is seen before, just increment the count by the
      // number of insts in basic block.
      BBInfo &info = map_itr->second;
      info.count += currentBBVInstCount;
    }
    currentBBVInstCount = 0;

    // Reached end of interval if the sum of the current inst count
    // (intervalCount) and the excessive inst count from the previous
    // interval (intervalDrift) is greater than/equal to the interval size.
    if (intervalCount + intervalDrift >= intervalSize) {
      // summarize interval and display BBV info
      std::vector<std::pair<uint64_t, uint64_t> > counts;
      for (auto map_itr = bbMap.begin(); map_itr != bbMap.end();
           ++map_itr) {
        BBInfo &info = map_itr->second;
        if (info.count != 0) {
          counts.push_back(std::make_pair(info.id, info.count));
          info.count = 0;
        }
      }
      std::sort(counts.begin(), counts.end());

      // Print output BBV info
      *simpointStream->stream() << "T";
      for (auto cnt_itr = counts.begin(); cnt_itr != counts.end();
           ++cnt_itr) {
        *simpointStream->stream() << ":" << cnt_itr->first
                                  << ":" << cnt_itr->second << " ";
      }
      *simpointStream->stream() << "\n";
      Logsp("Simpoint profilied %lu instrs", intervalCount);

      intervalDrift = (intervalCount + intervalDrift) - intervalSize;
      intervalCount = 0;
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
#endif

}
