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

#include <common.h>
#ifdef CONFIG_RVV

#include "vcompute_impl.h"
#include "vcommon.h"

def_EHelper(vadd) {
  ARTHI(ADD, SIGNED)
}

def_EHelper(vsub) {
  Assert(s->src_vmode != SRC_VI, "vsub.vi not supported\n");
  ARTHI(SUB, SIGNED)
}

def_EHelper(vrsub) {
  Assert(s->src_vmode != SRC_VV, "vrsub.vv not supported\n");
  ARTHI(RSUB, SIGNED)
}

def_EHelper(vminu) {
  ARTHI(MINU, UNSIGNED)
}

def_EHelper(vmin) {
  ARTHI(MIN, SIGNED)
}

def_EHelper(vmaxu) {
  ARTHI(MAXU, UNSIGNED)
}

def_EHelper(vmax) {
  ARTHI(MAX, SIGNED)
}

def_EHelper(vand) {
  ARTHI(AND, SIGNED)
}

def_EHelper(vor) {
  ARTHI(OR, SIGNED)
}

def_EHelper(vxor) {
  ARTHI(XOR, SIGNED)
}

def_EHelper(vrgather) {
  require_vector(true);
  double vflmul = compute_vflmul();
  require_aligned(id_dest->reg, vflmul);
  require_aligned(id_src2->reg, vflmul);
  if (id_dest->reg == id_src2->reg) {
    longjmp_exception(EX_II);
  }
  if (s->src_vmode == SRC_VV) {
    require_aligned(id_src->reg, vflmul);
    if (id_dest->reg == id_src->reg) {
      longjmp_exception(EX_II);
    }
  }
  require_vm(s);
  PERM(RGATHER)
}

def_EHelper(vrgatherei16) {
  require_vector(true);
  int vsew = 8 << vtype->vsew;
  double vflmul = compute_vflmul();
  double vemul = 16.0 / vsew * vflmul;
  if (vemul < 0.125 || vemul > 8) {
    longjmp_exception(EX_II);
  }
  require_aligned(id_dest->reg, vflmul);
  require_aligned(id_src2->reg, vflmul);
  require_aligned(id_src->reg, vemul);
  require_noover(id_dest->reg, vflmul, id_src->reg, vemul);
  if (id_dest->reg == id_src2->reg) {
    longjmp_exception(EX_II);
  }
  require_vm(s);
  PERM(RGATHEREI16)
}

def_EHelper(vslideup) {
  vector_slide_check(s, true);
  PERM(SLIDEUP)
}

def_EHelper(vslidedown) {
  vector_slide_check(s, false);
  PERM(SLIDEDOWN)
}

def_EHelper(vslide1up) {
  vector_slide_check(s, true);
  PERM(SLIDE1UP)
}

def_EHelper(vslide1down) {
  vector_slide_check(s, false);
  PERM(SLIDE1DOWN)
}

def_EHelper(vadc) {
  ARTHI(ADC, SIGNED)
}

def_EHelper(vmadc) {
  ARTHI_MASK(MADC, SIGNED)
}

def_EHelper(vsbc) {
  ARTHI(SBC, SIGNED)
}

def_EHelper(vmsbc) {
  ARTHI_MASK(MSBC, SIGNED)
}

def_EHelper(vmerge) {
  if (s->vm == 1) {
    // when vm is 1, it's a vmv instruction
    if (id_src2->reg != 0) {
      // The first operand specifier (vs2) must contain v0
      longjmp_exception(EX_II);
    }
  }
  ARTHI(MERGE, SIGNED)
}

def_EHelper(vmseq) {
  ARTHI_MASK(MSEQ, SIGNED)
}

def_EHelper(vmsne) {
  ARTHI_MASK(MSNE, SIGNED)
}

def_EHelper(vmsltu) {
  Assert(s->src_vmode != SRC_VI, "vmsltu not supprt SRC_VI\n");
  ARTHI_MASK(MSLTU, UNSIGNED)
}

def_EHelper(vmslt) {
  Assert(s->src_vmode != SRC_VI, "vmslt not supprt SRC_VI\n");
  ARTHI_MASK(MSLT, SIGNED)
}

def_EHelper(vmsleu) {
  ARTHI_MASK(MSLEU, UNSIGNED)
}

def_EHelper(vmsle) {
  ARTHI_MASK(MSLE, SIGNED);
}

def_EHelper(vmsgtu) {
  Assert(s->src_vmode != SRC_VV, "vmsgtu not support SRC_VV\n");
  ARTHI_MASK(MSGTU, UNSIGNED)
}

def_EHelper(vmsgt) {
  Assert(s->src_vmode != SRC_VV, "vmsgt not support SRC_VV\n");
  ARTHI_MASK(MSGT, SIGNED)
}

def_EHelper(vsaddu) {
  ARTHI(SADDU, UNSIGNED)
}

def_EHelper(vsadd) {
  ARTHI(SADD, SIGNED)
}

def_EHelper(vssubu) {
  ARTHI(SSUBU, UNSIGNED)
}

def_EHelper(vssub) {
  ARTHI(SSUB, SIGNED)
}

def_EHelper(vaadd) {
  ARTHI(AADD, SIGNED)
}

def_EHelper(vaaddu) {
  ARTHI(AADD, UNSIGNED)
}

def_EHelper(vsll) {
  ARTHI(SLL, UNSIGNED)
}

def_EHelper(vasub) {
  ARTHI(ASUB, SIGNED)
}

def_EHelper(vasubu) {
  ARTHI(ASUB, UNSIGNED)
}

def_EHelper(vsmul) {
  ARTHI(SMUL, SIGNED)
}

def_EHelper(vsrl) {
  ARTHI(SRL, UNSIGNED)
}

def_EHelper(vsra) {
  ARTHI(SRA, UNSIGNED)
}

def_EHelper(vssrl) {
  ARTHI(SSRL, UNSIGNED)
}

def_EHelper(vssra) {
  ARTHI(SSRA, SIGNED)
}

def_EHelper(vnsrl) {
  ARTHI_NARROW(SRL, UNSIGNED, 1)
}

def_EHelper(vnsra) {
  ARTHI_NARROW(SRA, UNSIGNED, 1)
}

def_EHelper(vnclipu) {
  ARTHI_NARROW(NCLIPU, UNSIGNED, 1)
}

def_EHelper(vnclip) {
  ARTHI_NARROW(NCLIP, SIGNED, 1)
}

def_EHelper(vwredsumu) {
  WREDUCTION(REDSUM, UNSIGNED);
}

def_EHelper(vwredsum) {
  WREDUCTION(REDSUM, SIGNED);
}

def_EHelper(vdotu) {
  longjmp_exception(EX_II);
}

def_EHelper(vdot) {
  longjmp_exception(EX_II);
}

def_EHelper(vwsmaccu) {
  longjmp_exception(EX_II);
}

def_EHelper(vwsmacc) {
  longjmp_exception(EX_II);
}

def_EHelper(vwsmaccsu) {
  longjmp_exception(EX_II);
}

def_EHelper(vwsmaccus) {
  longjmp_exception(EX_II);
}


//op-m
def_EHelper(vredsum) {
  REDUCTION(REDSUM, SIGNED);
}

def_EHelper(vredand) {
  REDUCTION(REDAND, UNSIGNED);
}

def_EHelper(vredor) {
  REDUCTION(REDOR, UNSIGNED);
}

def_EHelper(vredxor) {
  REDUCTION(REDXOR, UNSIGNED);
}

def_EHelper(vredminu) {
  REDUCTION(REDMINU, UNSIGNED);
}

def_EHelper(vredmin) {
  REDUCTION(REDMIN, SIGNED);
}

def_EHelper(vredmaxu) {
  REDUCTION(REDMAXU, UNSIGNED);
}

def_EHelper(vredmax) {
  REDUCTION(REDMAX, SIGNED);
}

/*
The integer scalar read/write instructions transfer a single value between a
scalar x register and element 0 of a vector register. The instructions ignore
LMUL and vector register groups.
*/
def_EHelper(vmvsx) {
  require_vector(true);
  if (s->vm == 0) {
    longjmp_exception(EX_II);
  }
  if (vstart->val < vl->val) {
    rtl_lr(s, &(id_src->val), id_src1->reg, 4);
    rtl_mv(s, s1, &id_src->val); 
    rtl_sext(s, s1, s1, 1 << vtype->vsew);
    set_vreg(id_dest->reg, 0, *s1, vtype->vsew, vtype->vlmul, 0);
    if (RVV_AGNOSTIC) {
      if(vtype->vta) {
        for (int idx = 8 << vtype->vsew; idx < VLEN; idx++) {
          set_mask(id_dest->reg, idx, 1, vtype->vsew, vtype->vlmul);
        }
      }
    }
  }
  vstart->val = 0;
}

def_EHelper(vmvxs) {
  require_vector(true);
  if (s->vm == 0) {
    longjmp_exception(EX_II);
  }
  get_vreg(id_src2->reg, 0, s0, vtype->vsew, vtype->vlmul, 1, 0);
  rtl_sext(s, s0, s0, 8);
  rtl_sr(s, id_dest->reg, s0, 8);
  vstart->val = 0;
}

def_EHelper(vmvnr) {
  require_vector(true);
  rtl_li(s, s1, s->isa.instr.v_opimm.v_imm5);
  int NREG = (*s1) + 1;
  int len = (VLEN >> 6) * NREG;
  int vlmul = 0;
  while (NREG > 1) {
    NREG = NREG >> 1;
    vlmul++;
  }
  for (int i = 0; i < len; i++) {
    get_vreg(id_src2->reg, i, s0, 3, vlmul, 1, 1);
    set_vreg(id_dest->reg, i, *s0, 3, vlmul, 1);
  }
  vstart->val = 0;
}

def_EHelper(vpopc) {
  require_vector(true);
  if(vstart->val != 0)
    check_vstart_exception(s);
  
  rtl_li(s, s1, 0);
  for(int idx = vstart->val; idx < vl->val; idx ++) {
        // mask
    rtlreg_t mask = get_mask(0, idx, vtype->vsew, vtype->vlmul);
    if(s->vm == 0 && mask == 0)
      continue;

    *s0 = get_mask(id_src2->reg, idx, vtype->vsew, vtype->vlmul);
    *s0 &= 1;

    if(*s0 == 1)
      rtl_addi(s, s1, s1, 1);
  }
  rtl_sr(s, id_dest->reg, s1, 4);
  vstart->val = 0;
}

def_EHelper(vfirst) {
  require_vector(true);
  if(vstart->val != 0)
    check_vstart_exception(s);

  int pos = -1;
  for(int idx = vstart->val; idx < vl->val; idx ++) {
    rtlreg_t mask = get_mask(0, idx, vtype->vsew, vtype->vlmul);
    if (s->vm || mask) {
      *s0 = get_mask(id_src2->reg, idx, vtype->vsew, vtype->vlmul);
      *s0 &= 1;
      if(*s0 == 1) {
        pos = idx;
        break;
      }
    }
  }
  rtl_li(s, s1, pos);
  rtl_sr(s, id_dest->reg, s1, 4);
  vstart->val = 0;
}

def_EHelper(vmsbf) {
  require_vector(true);
  require_vm(s);
  if (id_dest->reg == id_src2->reg) {
    longjmp_exception(EX_II);
  }
  if (vstart->val != 0) {
    // The vmsbf instruction will raise an illegal instruction exception if vstart is non-zero
    longjmp_exception(EX_II);
  }

  if (vl->val != 0) {
    // when vl = 0, do nothing
    bool first_one = false;
    for(int idx = vstart->val; idx < vl->val; idx ++) {
      rtlreg_t mask = get_mask(0, idx, vtype->vsew, vtype->vlmul);
      if(s->vm == 0 && mask == 0) {
        // it need v0 mask, but this element is not choosed by v0
        // if vma, set 1; others, continue
        if (RVV_AGNOSTIC) {
          if (vtype->vma) {
            set_mask(id_dest->reg, idx, 1, vtype->vsew, vtype->vlmul);
          }
        }
        continue;
      }

      // s->vm == 1: donot need v0 mask
      // or
      // s->vm == 0 && mask == 1: this element is choosed by v0

      *s0 = get_mask(id_src2->reg, idx, vtype->vsew, vtype->vlmul);
      *s0 &= 1;

      if(!first_one && *s0 == 1) {
        first_one = true;
      }

      if(first_one) {
        set_mask(id_dest->reg, idx, 0, vtype->vsew, vtype->vlmul);
      } else{
        set_mask(id_dest->reg, idx, 1, vtype->vsew, vtype->vlmul);
      }
    }
    /* The tail elements in the destination mask register are updated under a tail-agnostic policy. */
    if (RVV_AGNOSTIC) {
      for (int idx = vl->val; idx < VLEN; idx++) {
        set_mask(id_dest->reg, idx, 1, vtype->vsew, vtype->vlmul);
      }
    }
    vstart->val = 0;
  }
}

def_EHelper(vmsof) {
  require_vector(true);
  require_vm(s);
  if (id_dest->reg == id_src2->reg) {
    longjmp_exception(EX_II);
  }
  if (vstart->val != 0) {
    // The vmsof instruction will raise an illegal instruction exception if vstart is non-zero
    longjmp_exception(EX_II);
  }

  if (vl->val != 0) {
    // when vl = 0, do nothing
    bool first_one = false;
    for(int idx = vstart->val; idx < vl->val; idx ++) {
      rtlreg_t mask = get_mask(0, idx, vtype->vsew, vtype->vlmul);
      if(s->vm == 0 && mask == 0) {
        if (RVV_AGNOSTIC) {
          if (vtype->vma) {
            set_mask(id_dest->reg, idx, 1, vtype->vsew, vtype->vlmul);
          }
        }
        continue;
      }

      *s0 = get_mask(id_src2->reg, idx, vtype->vsew, vtype->vlmul);
      *s0 &= 1;

      if(!first_one && *s0 == 1) {
        set_mask(id_dest->reg, idx, 1, vtype->vsew, vtype->vlmul);
        first_one = true;
        continue;
      }
      set_mask(id_dest->reg, idx, 0, vtype->vsew, vtype->vlmul);
    }
    if (RVV_AGNOSTIC) {
      for (int idx = vl->val; idx < VLEN; idx++) {
        set_mask(id_dest->reg, idx, 1, vtype->vsew, vtype->vlmul);
      }
    }
    vstart->val = 0;
  }
}

def_EHelper(vmsif) {
  require_vector(true);
  require_vm(s);
  if (id_dest->reg == id_src2->reg) {
    longjmp_exception(EX_II);
  }
  if (vstart->val != 0) {
    // The vmsof instruction will raise an illegal instruction exception if vstart is non-zero
    longjmp_exception(EX_II);
  }

  if (vl->val != 0) {
    // when vl = 0, do nothing
    bool first_one = false;
    for(int idx = vstart->val; idx < vl->val; idx ++) {
      rtlreg_t mask = get_mask(0, idx, vtype->vsew, vtype->vlmul);
      if(s->vm == 0 && mask == 0) {
        if (RVV_AGNOSTIC) {
          if (vtype->vma) {
            set_mask(id_dest->reg, idx, 1, vtype->vsew, vtype->vlmul);
          }
        }
        continue;
      }

      *s0 = get_mask(id_src2->reg, idx, vtype->vsew, vtype->vlmul);
      *s0 &= 1;

      if(first_one) {
        set_mask(id_dest->reg, idx, 0, vtype->vsew, vtype->vlmul);
      } else{
        set_mask(id_dest->reg, idx, 1, vtype->vsew, vtype->vlmul);
      }

      if(!first_one && *s0 == 1) {
        first_one = true;
      }
    }
    if (RVV_AGNOSTIC) {
      for (int idx = vl->val; idx < VLEN; idx++) {
        set_mask(id_dest->reg, idx, 1, vtype->vsew, vtype->vlmul);
      }
    }
    vstart->val = 0;
  }
}

def_EHelper(viota) {
  require_vector(true);
  require_vm(s);
  double vflmul = compute_vflmul();
  require_aligned(id_dest->reg, vflmul);
  require_noover(id_dest->reg, vflmul, id_src2->reg, 1);

  if(!check_vstart_exception(s)) {
    rtl_li(s, s1, 0);
    for(int idx = vstart->val; idx < vl->val; idx ++) {
      rtlreg_t mask = get_mask(0, idx, vtype->vsew, vtype->vlmul);
      if(s->vm == 0 && mask == 0) {
        if (RVV_AGNOSTIC) {
          if (vtype->vma) {
            *s2 = (uint64_t) -1;
            set_vreg(id_dest->reg, idx, *s2, vtype->vsew, vtype->vlmul, 1);
            continue;
          }
        }
        continue;
      }

      *s0 = get_mask(id_src2->reg, idx, vtype->vsew, vtype->vlmul);
      *s0 &= 1;

      set_vreg(id_dest->reg, idx, *s1, vtype->vsew, vtype->vlmul, 1);

      if(*s0 == 1) {
        rtl_addi(s, s1, s1, 1);
      }
    }
    if (RVV_AGNOSTIC) {
      if(vtype->vta) {
        int vlmax = get_vlen_max(vtype->vsew, vtype->vlmul, 0);
        for(int idx = vl->val; idx < vlmax; idx++) {
          *s1 = (uint64_t) -1;
          set_vreg(id_dest->reg, idx, *s1, vtype->vsew, vtype->vlmul, 1);
        }
      }
    }
  }
  vstart->val = 0;
}

def_EHelper(vid) {
  require_vector(true);
  require_vm(s);
  if (id_src2->reg != 0) {
    longjmp_exception(EX_II);
  }
  double vflmul = compute_vflmul();
  require_aligned(id_dest->reg, vflmul);

  if(!check_vstart_exception(s)) {
    for(int idx = 0; idx < vl->val; idx ++) {
      // mask
      rtlreg_t mask = get_mask(0, idx, vtype->vsew, vtype->vlmul);
      // Masking does not change the index value written to active elements.
      if(s->vm == 0 && mask == 0) {
        if (RVV_AGNOSTIC) {
          if (vtype->vma) {
            *s2 = (uint64_t) -1;
            set_vreg(id_dest->reg, idx, *s2, vtype->vsew, vtype->vlmul, 1);
            continue;
          }
        }
        continue;
      }

      rtl_li(s, s1, idx);
      set_vreg(id_dest->reg, idx, *s1, vtype->vsew, vtype->vlmul, 1);
    }
    if (RVV_AGNOSTIC) {
      if(vtype->vta) {
        int vlmax = get_vlen_max(vtype->vsew, vtype->vlmul, 0);
        for(int idx = vl->val; idx < vlmax; idx++) {
          *s1 = (uint64_t) -1;
          set_vreg(id_dest->reg, idx, *s1, vtype->vsew, vtype->vlmul, 1);
        }
      }
    }
  }
  vstart->val = 0;
}

def_EHelper(vzextvf8) {
  require_vector(true);
  require_vm(s);
  if (id_dest->reg == id_src2->reg) {
    longjmp_exception(EX_II);
  }
  double vflmul = compute_vflmul();
  double vemul = vflmul / 8;
  if (vemul < 0.125 || vemul > 8) {
    longjmp_exception(EX_II);
  }
  require_aligned(id_dest->reg, vflmul);
  require_aligned(id_src2->reg, vemul);
  if (vemul < 1) {
    require_noover(id_dest->reg, vflmul, id_src2->reg, vemul);
  } else {
    require_noover_widen(id_dest->reg, vflmul, id_src2->reg, vemul);
  }
  ARTHI_NARROW(VEXT, UNSIGNED, -3);
}

def_EHelper(vsextvf8) {
  require_vector(true);
  require_vm(s);
  if (id_dest->reg == id_src2->reg) {
    longjmp_exception(EX_II);
  }
  double vflmul = compute_vflmul();
  double vemul = vflmul / 8;
  if (vemul < 0.125 || vemul > 8) {
    longjmp_exception(EX_II);
  }
  require_aligned(id_dest->reg, vflmul);
  require_aligned(id_src2->reg, vemul);
  if (vemul < 1) {
    require_noover(id_dest->reg, vflmul, id_src2->reg, vemul);
  } else {
    require_noover_widen(id_dest->reg, vflmul, id_src2->reg, vemul);
  }
  ARTHI_NARROW(VEXT, SIGNED, -3);
}

def_EHelper(vzextvf4) {
  require_vector(true);
  require_vm(s);
  if (id_dest->reg == id_src2->reg) {
    longjmp_exception(EX_II);
  }
  double vflmul = compute_vflmul();
  double vemul = vflmul / 4;
  if (vemul < 0.125 || vemul > 8) {
    longjmp_exception(EX_II);
  }
  require_aligned(id_dest->reg, vflmul);
  require_aligned(id_src2->reg, vemul);
  if (vemul < 1) {
    require_noover(id_dest->reg, vflmul, id_src2->reg, vemul);
  } else {
    require_noover_widen(id_dest->reg, vflmul, id_src2->reg, vemul);
  }
  ARTHI_NARROW(VEXT, UNSIGNED, -2);
}

def_EHelper(vsextvf4) {
  require_vector(true);
  require_vm(s);
  if (id_dest->reg == id_src2->reg) {
    longjmp_exception(EX_II);
  }
  double vflmul = compute_vflmul();
  double vemul = vflmul / 4;
  if (vemul < 0.125 || vemul > 8) {
    longjmp_exception(EX_II);
  }
  require_aligned(id_dest->reg, vflmul);
  require_aligned(id_src2->reg, vemul);
  if (vemul < 1) {
    require_noover(id_dest->reg, vflmul, id_src2->reg, vemul);
  } else {
    require_noover_widen(id_dest->reg, vflmul, id_src2->reg, vemul);
  }
  ARTHI_NARROW(VEXT, SIGNED, -2);
}

def_EHelper(vzextvf2) {
  require_vector(true);
  require_vm(s);
  if (id_dest->reg == id_src2->reg) {
    longjmp_exception(EX_II);
  }
  double vflmul = compute_vflmul();
  double vemul = vflmul / 2;
  if (vemul < 0.125 || vemul > 8) {
    longjmp_exception(EX_II);
  }
  require_aligned(id_dest->reg, vflmul);
  require_aligned(id_src2->reg, vemul);
  if (vemul < 1) {
    require_noover(id_dest->reg, vflmul, id_src2->reg, vemul);
  } else {
    require_noover_widen(id_dest->reg, vflmul, id_src2->reg, vemul);
  }
  ARTHI_NARROW(VEXT, UNSIGNED, -1);
}

def_EHelper(vsextvf2) {
  require_vector(true);
  require_vm(s);
  if (id_dest->reg == id_src2->reg) {
    longjmp_exception(EX_II);
  }
  double vflmul = compute_vflmul();
  double vemul = vflmul / 2;
  if (vemul < 0.125 || vemul > 8) {
    longjmp_exception(EX_II);
  }
  require_aligned(id_dest->reg, vflmul);
  require_aligned(id_src2->reg, vemul);
  if (vemul < 1) {
    require_noover(id_dest->reg, vflmul, id_src2->reg, vemul);
  } else {
    require_noover_widen(id_dest->reg, vflmul, id_src2->reg, vemul);
  }
  ARTHI_NARROW(VEXT, SIGNED, -1);
}

def_EHelper(vcompress) {
  require_vector(true);
  if (s->vm == 0) {
    longjmp_exception(EX_II);
  }
  double vflmul = compute_vflmul();
  require_aligned(id_dest->reg, vflmul);
  require_aligned(id_src2->reg, vflmul);
  if (id_dest->reg == id_src2->reg) {
    longjmp_exception(EX_II);
  }
  require_noover(id_dest->reg, vflmul, id_src->reg, 1);
  if(!check_vstart_exception(s)) {

    rtl_li(s, s1, 0);
    for(int idx = vstart->val; idx < vl->val; idx ++) {
      rtlreg_t mask = get_mask(id_src1->reg, idx, vtype->vsew, vtype->vlmul);

      if (mask == 0) {
          continue;
      }

      get_vreg(id_src2->reg, idx, s0, vtype->vsew, vtype->vlmul, 1, 1);

      set_vreg(id_dest->reg, *s1, *s0, vtype->vsew, vtype->vlmul, 1);

      rtl_addi(s, s1, s1, 1);
    }
    if (RVV_AGNOSTIC) {
      if(vtype->vta) {
        int vlmax = get_vlen_max(vtype->vsew, vtype->vlmul, 0);
        for(int idx = *s1; idx < vlmax; idx++) {
          *s1 = (uint64_t) -1;
          set_vreg(id_dest->reg, idx, *s1, vtype->vsew, vtype->vlmul, 1);
        }
      }
    }
  }
  vstart->val = 0;
}

def_EHelper(vmandnot) {
  MASKINSTR(MANDNOT)
}

def_EHelper(vmand) {
  MASKINSTR(MAND)
}

def_EHelper(vmor) {
  MASKINSTR(MOR)
}

def_EHelper(vmxor) {
  MASKINSTR(MXOR)
}

def_EHelper(vmornot) {
  MASKINSTR(MORNOT)
}

def_EHelper(vmnand) {
  MASKINSTR(MNAND)
}

def_EHelper(vmnor) {
  MASKINSTR(MNOR)
}

def_EHelper(vmxnor) {
  MASKINSTR(MXNOR);
}

def_EHelper(vdivu) {
  Assert(s->src_vmode != SRC_VI, "vdivu does not support SRC_VI\n");
  ARTHI(DIVU, UNSIGNED)
}

def_EHelper(vdiv) {
  Assert(s->src_vmode != SRC_VI, "vdiv does not support SRC_VI\n");
  ARTHI(DIV, SIGNED)
}

def_EHelper(vremu) {
  Assert(s->src_vmode != SRC_VI, "vremu does not support SRC_VI\n");
  ARTHI(REMU, UNSIGNED)
}

def_EHelper(vrem) {
  Assert(s->src_vmode != SRC_VI, "vrem does not support SRC_VI\n");
  ARTHI(REM, SIGNED)
}

def_EHelper(vmulhu) {
  Assert(s->src_vmode != SRC_VI, "vmulhu does not support SRC_VI\n");
  ARTHI(MULHU, UNSIGNED)
}

def_EHelper(vmul) {
  Assert(s->src_vmode != SRC_VI, "vmul does not support SRC_VI\n");
  ARTHI(MUL, SIGNED)
}

def_EHelper(vmulhsu) {
  Assert(s->src_vmode != SRC_VI, "vmulhsu does not support SRC_VI\n");
  ARTHI(MULHSU, UNSIGNED)
}

def_EHelper(vmulh) {
  Assert(s->src_vmode != SRC_VI, "vmulh does not support SRC_VI\n");
  ARTHI(MULH, SIGNED)
}

def_EHelper(vmadd) {
  ARTHI(MADD, SIGNED)
}

def_EHelper(vnmsub) {
  ARTHI(NMSUB, SIGNED)
}

def_EHelper(vmacc) {
  ARTHI(MACC, SIGNED)
}

def_EHelper(vnmsac) {
  ARTHI(NMSAC, SIGNED)
}

def_EHelper(vwaddu) {
  ARTHI_WIDE(ADD, UNSIGNED)
}

def_EHelper(vwadd) {
  ARTHI_WIDE(ADD, SIGNED)
}

def_EHelper(vwsubu) {
  ARTHI_WIDE(SUB, UNSIGNED)
}

def_EHelper(vwsub) {
  ARTHI_WIDE(SUB, SIGNED)
}

def_EHelper(vwaddu_w) {
  arthimetic_instr(ADD, UNSIGNED, 1, 1, 0, s);
}

def_EHelper(vwadd_w) {
  arthimetic_instr(ADD, SIGNED, 1, 1, 0, s);
}

def_EHelper(vwsubu_w) {
  arthimetic_instr(SUB, UNSIGNED, 1, 1, 0, s);
}

def_EHelper(vwsub_w) {
  arthimetic_instr(SUB, SIGNED, 1, 1, 0, s);
}

def_EHelper(vwmulu) {
  ARTHI_WIDE(MUL, UNSIGNED)
}

def_EHelper(vwmulsu) {
  ARTHI_WIDE(MULSU, UNSIGNED)
}

def_EHelper(vwmul) {
  ARTHI_WIDE(MUL, SIGNED)
}

def_EHelper(vwmaccu) {
  ARTHI_WIDE(MACC, UNSIGNED)
}

def_EHelper(vwmacc) {
  ARTHI_WIDE(MACC, SIGNED)
}

def_EHelper(vwmaccsu) {
  ARTHI_WIDE(MACCSU, UNSIGNED)
}

def_EHelper(vwmaccus) {
  ARTHI_WIDE(MACCUS, UNSIGNED)
}

def_EHelper(vfadd) {
  FLOAT_ARTHI(FADD, UNSIGNED)
}

def_EHelper(vfredusum) {
#ifdef CONFIG_DIFFTEST
  FREDUCTION(FREDUSUM)    // use ordered reduction
#else
  float_reduction_computing(s);   // when NEMU is ref, use unordered reduction which is same as XiangShan
#endif
}

def_EHelper(vfsub) {
  FLOAT_ARTHI(FSUB, UNSIGNED)
}

def_EHelper(vfredosum) {
  FREDUCTION(FREDOSUM)
}

def_EHelper(vfmin) {
  FLOAT_ARTHI(FMIN, UNSIGNED)
}

def_EHelper(vfredmin) {
  FREDUCTION(FREDMIN)
}

def_EHelper(vfmax) {
  FLOAT_ARTHI(FMAX, UNSIGNED)
}

def_EHelper(vfredmax) {
  FREDUCTION(FREDMAX)
}

def_EHelper(vfsgnj) {
  FLOAT_ARTHI(FSGNJ, UNSIGNED)
}

def_EHelper(vfsgnjn) {
  FLOAT_ARTHI(FSGNJN, UNSIGNED)
}

def_EHelper(vfsgnjx) {
  FLOAT_ARTHI(FSGNJX, UNSIGNED)
}

def_EHelper(vfslide1up) {
  vector_slide_check(s, true);
  FLOAT_ARTHI_NOCHECK(FSLIDE1UP)
}

def_EHelper(vfslide1down) {
  vector_slide_check(s, false);
  FLOAT_ARTHI_NOCHECK(FSLIDE1DOWN)
}

def_EHelper(vfmvfs) {
  require_vector(true);
  if (s->vm == 0) {
    longjmp_exception(EX_II);
  }
  if (vtype->vsew <= 1) {
    Loge("ZVFH not supported");
    longjmp_exception(EX_II);
  }
  get_vreg(id_src2->reg, 0, s0, vtype->vsew, vtype->vlmul, 1, 0);
  if (vtype->vsew < 3) {
      *s0 = *s0 | (UINT64_MAX << (8 << vtype->vsew));
  }
  rtl_mv(s, &fpreg_l(id_dest->reg), s0);
  vstart->val = 0;
}

def_EHelper(vfmvsf) {
  require_vector(true);
  if (s->vm == 0) {
    longjmp_exception(EX_II);
  }
  if (vtype->vsew <= 1) {
    Loge("ZVFH not supported");
    longjmp_exception(EX_II);
  }
  if (vl->val > 0 && vstart->val < vl->val) {
    rtl_mv(s, s1, &fpreg_l(id_src1->reg)); // f[rs1]
    check_isFpCanonicalNAN(s1, vtype->vsew);
    set_vreg(id_dest->reg, 0, *s1, vtype->vsew, vtype->vlmul, 0);
    if (RVV_AGNOSTIC) {
      if(vtype->vta) {
        for (int idx = 8 << vtype->vsew; idx < VLEN; idx++) {
          set_mask(id_dest->reg, idx, 1, vtype->vsew, vtype->vlmul);
        }
      }
    }
  }
  vstart->val = 0;
}

def_EHelper(vfcvt_xufv) {
  FLOAT_ARTHI(FCVT_XUF, UNSIGNED)
}

def_EHelper(vfcvt_xfv) {
  FLOAT_ARTHI(FCVT_XF, UNSIGNED)
}

def_EHelper(vfcvt_fxuv) {
  FLOAT_ARTHI(FCVT_FXU, UNSIGNED)
}

def_EHelper(vfcvt_fxv) {
  FLOAT_ARTHI(FCVT_FX, SIGNED)
}

def_EHelper(vfcvt_rtz_xufv) {
  FLOAT_ARTHI(FCVT_RTZ_XUF, UNSIGNED)
}

def_EHelper(vfcvt_rtz_xfv) {
  FLOAT_ARTHI(FCVT_RTZ_XF, UNSIGNED)
}

def_EHelper(vfwcvt_xufv) {
  FLOAT_ARTHI_DWIDE(FWCVT_XUF, UNSIGNED)
}

def_EHelper(vfwcvt_xfv) {
  FLOAT_ARTHI_DWIDE(FWCVT_XF, UNSIGNED)
}

def_EHelper(vfwcvt_fxuv) {
  FLOAT_ARTHI_DWIDE_X2F(FWCVT_FXU, UNSIGNED)
}

def_EHelper(vfwcvt_fxv) {
  FLOAT_ARTHI_DWIDE_X2F(FWCVT_FX, SIGNED)
}

def_EHelper(vfwcvt_ffv) {
  FLOAT_ARTHI_DWIDE(FWCVT_FF, UNSIGNED)
}

def_EHelper(vfwcvt_rtz_xufv) {
  FLOAT_ARTHI_DWIDE(FWCVT_RTZ_XUF, UNSIGNED)
}

def_EHelper(vfwcvt_rtz_xfv) {
  FLOAT_ARTHI_DWIDE(FWCVT_RTZ_XF, UNSIGNED)
}

def_EHelper(vfncvt_xufw) {
  FLOAT_ARTHI_DNARROW_F2X(FNCVT_XUF, UNSIGNED)
}

def_EHelper(vfncvt_xfw) {
  FLOAT_ARTHI_DNARROW_F2X(FNCVT_XF, UNSIGNED)
}

def_EHelper(vfncvt_fxuw) {
  FLOAT_ARTHI_DNARROW(FNCVT_FXU, UNSIGNED)
}

def_EHelper(vfncvt_fxw) {
  FLOAT_ARTHI_DNARROW(FNCVT_FX, SIGNED)
}

def_EHelper(vfncvt_ffw) {
  FLOAT_ARTHI_DNARROW(FNCVT_FF, UNSIGNED)
}

def_EHelper(vfncvt_rod_ffw) {
  FLOAT_ARTHI_DNARROW(FNCVT_ROD_FF, UNSIGNED)
}

def_EHelper(vfncvt_rtz_xufw) {
  FLOAT_ARTHI_DNARROW_F2X(FNCVT_RTZ_XUF, UNSIGNED)
}

def_EHelper(vfncvt_rtz_xfw) {
  FLOAT_ARTHI_DNARROW_F2X(FNCVT_RTZ_XF, UNSIGNED)
}

def_EHelper(vfsqrt_v) {
  FLOAT_ARTHI(FSQRT, UNSIGNED)
}

def_EHelper(vfrsqrt7_v) {
  FLOAT_ARTHI(FRSQRT7, UNSIGNED)
}

def_EHelper(vfrec7_v) {
  FLOAT_ARTHI(FREC7, UNSIGNED)
}

def_EHelper(vfclass_v) {
  FLOAT_ARTHI(FCLASS, UNSIGNED)
}

def_EHelper(vfmerge) {
  if (s->vm == 1) {
    // when vm is 1, it's a vfmv instruction
    if (id_src2->reg != 0) {
      // The first operand specifier (vs2) must contain v0
      longjmp_exception(EX_II);
    }
  }
  FLOAT_ARTHI(FMERGE, UNSIGNED)
}

def_EHelper(vmfeq) {
  FLOAT_ARTHI_MASK(MFEQ)
}

def_EHelper(vmfle) {
  FLOAT_ARTHI_MASK(MFLE)
}

def_EHelper(vmflt) {
  FLOAT_ARTHI_MASK(MFLT)
}

def_EHelper(vmfne) {
  FLOAT_ARTHI_MASK(MFNE)
}

def_EHelper(vmfgt) {
  FLOAT_ARTHI_MASK(MFGT)
}

def_EHelper(vmfge) {
  FLOAT_ARTHI_MASK(MFGE)
}

def_EHelper(vfdiv) {
  FLOAT_ARTHI(FDIV, UNSIGNED)
}

def_EHelper(vfrdiv) {
  FLOAT_ARTHI(FRDIV, UNSIGNED)
}

def_EHelper(vfmul) {
  FLOAT_ARTHI(FMUL, UNSIGNED)
}

def_EHelper(vfrsub) {
  FLOAT_ARTHI(FRSUB, UNSIGNED)
}

def_EHelper(vfmadd) {
  FLOAT_ARTHI(FMADD, UNSIGNED)
}

def_EHelper(vfnmadd) {
  FLOAT_ARTHI(FNMADD, UNSIGNED)
}

def_EHelper(vfmsub) {
  FLOAT_ARTHI(FMSUB, UNSIGNED)
}

def_EHelper(vfnmsub) {
  FLOAT_ARTHI(FNMSUB, UNSIGNED)
}

def_EHelper(vfmacc) {
  FLOAT_ARTHI(FMACC, UNSIGNED)
}

def_EHelper(vfnmacc) {
  FLOAT_ARTHI(FNMACC, UNSIGNED)
}

def_EHelper(vfmsac) {
  FLOAT_ARTHI(FMSAC, UNSIGNED)
}

def_EHelper(vfnmsac) {
  FLOAT_ARTHI(FNMSAC, UNSIGNED)
}

def_EHelper(vfwadd) {
  FLOAT_ARTHI_SDWIDE(FADD)
}

def_EHelper(vfwredusum) {
  FWREDUCTION(FREDUSUM)
}

def_EHelper(vfwsub) {
  FLOAT_ARTHI_SDWIDE(FSUB)
}

def_EHelper(vfwredosum) {
  FWREDUCTION(FREDOSUM)
}

def_EHelper(vfwadd_w) {
  FLOAT_ARTHI_SWIDE(FADD)
}

def_EHelper(vfwsub_w) {
  FLOAT_ARTHI_SWIDE(FSUB)
}

def_EHelper(vfwmul) {
  FLOAT_ARTHI_SDWIDE(FMUL)
}

def_EHelper(vfwmacc) {
  FLOAT_ARTHI_SDWIDE(FMACC)
}

def_EHelper(vfwnmacc) {
  FLOAT_ARTHI_SDWIDE(FNMACC)
}

def_EHelper(vfwmsac) {
  FLOAT_ARTHI_SDWIDE(FMSAC)
}

def_EHelper(vfwnmsac) {
  FLOAT_ARTHI_SDWIDE(FNMSAC)
}

def_EHelper(vandn) {
  ARTHI(ANDN, SIGNED)
}

def_EHelper(vbrev_v) {
  ARTHI(BREV_V, UNSIGNED)
}

def_EHelper(vbrev8_v) {
  ARTHI(BREV8_V, UNSIGNED)
}

def_EHelper(vrev8_v) {
  ARTHI(REV8_V, UNSIGNED)
}

def_EHelper(vclz_v) {
  ARTHI(CLZ_V, UNSIGNED)
}

def_EHelper(vctz_v) {
  ARTHI(CTZ_V, UNSIGNED)
}

def_EHelper(vcpop_v) {
  ARTHI(CPOP_V, UNSIGNED)
}

def_EHelper(vrol) {
  ARTHI(ROL, UNSIGNED)
}

def_EHelper(vror) {
  ARTHI(ROR, UNSIGNED)
}

def_EHelper(vwsll) {
  ARTHI_WIDE(SLL, UNSIGNED)
}

#endif // CONFIG_RVV