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

static void csr_prepare() {
  cpu.mstatus = mstatus->val;
  cpu.mcause  = mcause->val;
  cpu.mepc    = mepc->val;

  cpu.sstatus = mstatus->val & SSTATUS_RMASK; // sstatus
  cpu.scause  = scause->val;
  cpu.sepc    = sepc->val;

  cpu.satp     = satp->val;
  cpu.mip      = mip->val;
  cpu.mie      = mie->val;
  cpu.mscratch = mscratch->val;
  cpu.sscratch = sscratch->val;
  cpu.mideleg  = mideleg->val;
  cpu.medeleg  = medeleg->val;
  cpu.mtval    = mtval->val;
  cpu.stval    = stval->val;
  cpu.mtvec    = mtvec->val;
  cpu.stvec    = stvec->val;
#ifdef CONFIG_RVV_010
  cpu.vtype   = vtype->val;
  cpu.vstart  = vstart->val;
  cpu.vxsat   = vxsat->val;
  cpu.vxrm    = vxrm->val;
  cpu.vl      = vl->val;
#endif // CONFIG_RVV_010
#ifdef CONFIG_RVH
  cpu.mtval2  = mtval2->val;
  cpu.mtinst  = mtinst->val;
  cpu.hstatus = hstatus->val;
  cpu.hideleg = hideleg->val;
  cpu.hedeleg = hedeleg->val;
  cpu.hcounteren = hcounteren->val;
  cpu.htval   = htval->val;
  cpu.htinst  = htinst->val;
  cpu.hgatp   = hgatp->val;
  cpu.vsstatus= vsstatus->val;
  cpu.vstvec  = vstvec->val;
  cpu.vsepc   = vsepc->val;
  cpu.vscause = vscause->val;
  cpu.vstval  = vstval->val;
  cpu.vsatp   = vsatp->val;
  cpu.vsscratch = vsscratch->val;
#endif
}

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
    #ifdef CONFIG_RVH
    #define check_reg(r) difftest_check_reg(str(r), pc, ref_r->r, cpu.r)
    check_reg(v);//virtualization mode
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
    #endif
    return false;
  }
  return true;
}

void isa_difftest_attach() {
  csr_prepare();
  ref_difftest_memcpy(CONFIG_MBASE, guest_to_host(CONFIG_MBASE), MEMORY_SIZE, DIFFTEST_TO_REF);
  ref_difftest_regcpy(&cpu, DIFFTEST_TO_REF);
}
