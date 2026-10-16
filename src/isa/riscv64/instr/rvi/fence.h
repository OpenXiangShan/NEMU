/***************************************************************************************
* Copyright (c) 2014-2021 Zihao Yu, Nanjing University
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

#ifdef CONFIG_AME_MEM_ACCESS_CHECK
#include <ame/svstore_queue_wrapper.h>
#endif // CONFIG_AME_MEM_ACCESS_CHECK

def_EHelper(fence_i) {
  IFNDEF(CONFIG_DIFFTEST_REF_NEMU, difftest_skip_dut(1, 2));
  set_sys_state_flag(SYS_STATE_FLUSH_TCACHE);
  rtl_priv_next(s);
}

def_EHelper(fence) {
#ifdef CONFIG_AME_MEM_ACCESS_CHECK
  // Normal FENCE orders W (also O for PBMT=IO RAM stores) before matrix RAM reads.
  uint32_t inst = s->isa.instr.val;
  if (BITS(inst, 31, 28) == 0 && (BITS(inst, 23, 20) & 2)) {
    svstore_queue_update_fence(BITS(inst, 27, 24));
  }
#endif
  IFNDEF(CONFIG_DIFFTEST_REF_NEMU, difftest_skip_dut(1, 2));
}

#ifdef CONFIG_RV_ZIHINTPAUSE
def_EHelper(pause) {
  IFNDEF(CONFIG_DIFFTEST_REF_NEMU, difftest_skip_dut(1, 2));
}
#endif //CONFIG_RV_ZIHINTPAUSE
