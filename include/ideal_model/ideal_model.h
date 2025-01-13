/***************************************************************************************
* Copyright (c) 2014-2021 Zihao Yu, Nanjing University
* Copyright (c) 2020-2022 Institute of Computing Technology, Chinese Academy of Sciences
* Copyright (c) 2024      Yibo Zhang, University of Science and Technology of China
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
#ifndef __CPU_IDEAL_MODEL_H__
#define __CPU_IDEAL_MODEL_H__

#include "common.h"
#include "difftest.h"
#include "isa.h"

// some function reference from difftest

enum CommDirection{
    GEM5_TO_IDEAL_MODEL,
    IDEAL_MODEL_TO_GEM5,
};

enum InstFlowRecoverType{
  RECOVER_BRANCHMISPRED,
  RECOVER_MEMORDERVIOLATION,
  RECOVER_OTHER
};

enum IdealModelWorkState{
  IM_WORK,
  IM_EXCEPTION_STOP,
  IM_SYSTEMOP_STOP,
  IM_NONSPEC_STOP,
  IM_ITER_STOP
};

#ifdef CONFIG_DEBUG_IDEAL_MODEL
static const char *IdealModelWorkStateStr[] = {
  "IM_WORK",
  "IM_EXCEPTION_STOP",
  "IM_SYSTEMOP_STOP",
  "IM_NONSPEC_STOP",
  "IM_ITER_STOP"
};
#endif

enum GEM5CommitType{
  GEM5_NORMAL,
  GEM5_SINGLECONTROL,
  GEM5_EXCEPTION,
  GEM5_SYSTEMOP,
  GEM5_NOSPEC
};



// ask from gem5
struct AskFromGEM5{
  uint64_t seq_no;
  uint64_t pc;
  uint64_t next_pc;

  // system op: gem5 nonspec and serialize after
  bool is_systemop;

  // nonspec: mmio ll/sc atomic
  bool is_nonspec;

  // load
  bool is_load;

  // store
  bool is_store;

  // need provide dest value
  // different instruction have
  // different value presentation
  // now i want only support value pred-inst
  bool need_provide_dest_value;
};

struct AnswerFromNemu{
  uint64_t dest_value;
  uint64_t mem_vaddr;
  uint64_t mem_paddr;
  bool is_mmio;
  int ideal_model_work_state;
};



#endif
