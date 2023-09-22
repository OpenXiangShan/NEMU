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

#include <isa.h>
#include <memory/paddr.h>
#include <cpu/difftest.h>
#include "../local-include/csr.h"
#include "../local-include/reg.h"
#include <difftest.h>

// csr_prepare() & csr_writeback() are used to maintain 
// a compact mirror of critical CSRs
// For processor difftest only 
#ifdef CONFIG_RVH
#define MIDELEG_FORCED_MASK ((1 << 12) | (1 << 10) | (1 << 6) | (1 << 2)) 
#endif //CONFIG_RVH

bool isa_difftest_checkregs(CPU_state *ref_r, vaddr_t pc) {
  csr_prepare();
  if(cpu.mip != ref_r->mip) ref_r->mip = cpu.mip; // ignore difftest for mip
  if (memcmp(&cpu.gpr[1], &ref_r->gpr[1], DIFFTEST_REG_SIZE - sizeof(cpu.gpr[0]))) {
    int i;    
    // do not check $0
    for (i = 1; i < ARRLEN(cpu.gpr); i ++) {
      difftest_check_reg(reg_name(i, 4), pc, ref_r->gpr[i]._64, cpu.gpr[i]._64);
    }
    difftest_check_reg("pc", pc, ref_r->pc, cpu.pc);
    #ifdef CONFIG_RVV
    for(i=0;i < ARRLEN(cpu.vr); i++){
      difftest_check_vreg(vreg_name(i, 8), pc, ref_r->vr[i]._64, cpu.vr[i]._64,VLEN/8);
    }
    #endif // CONFIG_RVV

    #define check_reg(r) difftest_check_reg(str(r), pc, ref_r->r, cpu.r)

    check_reg(mstatus   );
    check_reg(mcause    );
    check_reg(mepc      );
    check_reg(sstatus   );
    check_reg(scause    );
    check_reg(sepc      );
    check_reg(satp      );
    check_reg(mip       );
    check_reg(mie       );
    check_reg(mscratch  );
    check_reg(sscratch  );
    check_reg(mideleg   );
    check_reg(medeleg   );
    check_reg(mtval     );
    check_reg(stval     );
    check_reg(mtvec     );
    check_reg(stvec     );

    #ifdef CONFIG_RVV
    check_reg(vtype     );
    check_reg(vstart    );
    check_reg(vxsat     );
    check_reg(vxrm      );
    check_reg(vl        );
    check_reg(vcsr      );
    check_reg(vlenb     );
    #endif // CONFIG_RVV

    #ifdef CONFIG_RVH
    check_reg(v);//virtualization mode
    check_reg(mtval2    );
    check_reg(mtinst    );
    check_reg(hstatus   );
    check_reg(hideleg   );
    check_reg(hedeleg   );
    check_reg(hcounteren);
    check_reg(htval     );
    check_reg(htinst    );
    check_reg(hgatp     );
    check_reg(vsstatus  );
    check_reg(vstvec    );
    check_reg(vsepc     );
    check_reg(vscause   );
    check_reg(vstval    );
    check_reg(vsatp     );
    check_reg(vsscratch );
    #endif // CONFIG_RVH
    return false;
  }
  return true;
}

void isa_difftest_attach() {
  csr_prepare();
  ref_difftest_memcpy(CONFIG_MBASE, guest_to_host(CONFIG_MBASE), MEMORY_SIZE, DIFFTEST_TO_REF);
  ref_difftest_regcpy(&cpu, DIFFTEST_TO_REF);
}
