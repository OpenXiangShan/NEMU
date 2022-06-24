/***************************************************************************************
* Copyright (c) 2014-2021 Zihao Yu, Nanjing University
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

#include "../instr/pseudo.h"
#include "../instr/rvi/exec.h"
#include "../instr/rvc/exec.h"
#include "../instr/rvm/exec.h"
#include "../instr/rvf/exec.h"
#include "../instr/rvd/exec.h"
#include "../instr/rva/exec.h"
#include "../instr/priv/exec.h"
#ifdef CONFIG_RVB
#include "../instr/rvb/exec.h"
#endif
#ifdef CONFIG_RVK
#include "../instr/rvk/exec.h"
#endif
#ifdef CONFIG_RVV_010
#include "../instr/rvv/exec.h"
#endif
#include "../instr/special.h"
