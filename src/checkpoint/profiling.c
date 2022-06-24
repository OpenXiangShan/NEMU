/***************************************************************************************
* Copyright (c) 2020-2022 Institute of Computing Technology, Chinese Academy of Sciences
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

#include <checkpoint/profiling.h>

int profiling_state = NoProfiling;
bool checkpoint_taking = false;
bool checkpoint_restoring = false;
uint64_t checkpoint_interval = 0;

bool profiling_started = false;
bool force_cpt_mmode = false;

#ifdef CONFIG_SHARE
// empty definition on share
void simpoint_profiling(uint64_t pc, bool is_control, uint64_t abs_instr_count) {}
#endif 