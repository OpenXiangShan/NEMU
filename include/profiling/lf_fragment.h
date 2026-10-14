/***************************************************************************************
* Copyright (c) 2026 Institute of Computing Technology, Chinese Academy of Sciences
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

// BBV fragment written by a leader-follower follower for its slice.
//
// File layout (native endianness):
//   lf_fragment_header_t
//   nr_blocks x { uint64_t first_pc, last_pc }    blocks in first-publication
//                                                 order; local id = index + 1
//   nr_segments x { uint64_t closed, n, then n x { uint64_t local_id, count } }
//
// A segment holds the counts of one interval part, sorted by local id. Every
// segment but the last is closed (the serial profiler ended an interval
// there); the last one is the open tail of the slice.

#ifndef __PROFILING_LF_FRAGMENT_H__
#define __PROFILING_LF_FRAGMENT_H__

#include <stdint.h>

#define LF_FRAG_MAGIC "NEMUBBVF"
#define LF_FRAG_VERSION 1

typedef struct {
  char magic[8];
  uint32_t version, slice;
  uint64_t start_icount, end_icount;      // P_i, P_{i+1} (absolute counts)
  uint64_t start_profiled, end_profiled;  // instructions profiled before P_i, P_{i+1}
  uint64_t interval_size;
  uint64_t first_interval;                // index of the interval open at P_i
  uint64_t nr_blocks;
  uint64_t nr_segments;
  uint64_t run_ended;
} lf_fragment_header_t;

#endif // __PROFILING_LF_FRAGMENT_H__
