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

#include <isa/riscv64/instr/pseudo.h>
#include <isa/riscv64/instr/rvi/exec.h>
#include <isa/riscv64/instr/rvc/exec.h>
#include <isa/riscv64/instr/rvm/exec.h>
#ifndef CONFIG_FPU_NONE
#include <isa/riscv64/instr/rvf/exec.h>
#include <isa/riscv64/instr/rvd/exec.h>
#endif // CONFIG_FPU_NONE
#include <isa/riscv64/instr/rva/exec.h>
#include <isa/riscv64/instr/priv/exec.h>
#ifdef CONFIG_RVB
#include <isa/riscv64/instr/rvb/exec.h>
#endif
#ifdef CONFIG_RVK
#include <isa/riscv64/instr/rvk/exec.h>
#endif
#ifdef CONFIG_RVV
#include <isa/riscv64/instr/rvv/exec.h>
#endif
#ifdef CONFIG_RV_ZICOND
#include <isa/riscv64/instr/rvzicond/exec.h>
#endif
#ifdef CONFIG_RV_ZFH_MIN
#include <isa/riscv64/instr/rvzfh/exec.h>
#endif
#ifdef CONFIG_RV_CBO
#include <isa/riscv64/instr/rvcbo/exec.h>
#endif
#ifdef CONFIG_RV_ZFA
#include <isa/riscv64/instr/rvzfa/exec.h>
#endif
#include <isa/riscv64/instr/special.h>
