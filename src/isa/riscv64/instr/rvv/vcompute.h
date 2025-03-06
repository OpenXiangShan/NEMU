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
  ARITH(ADD, SIGNED)
}

def_EHelper(vsub) {
  Assert(s->src_vmode != SRC_VI, "vsub.vi not supported\n");
  ARITH(SUB, SIGNED)
}

def_EHelper(vrsub) {
  Assert(s->src_vmode != SRC_VV, "vrsub.vv not supported\n");
  ARITH(RSUB, SIGNED)
}

def_EHelper(vminu) {
  ARITH(MINU, UNSIGNED)
}

def_EHelper(vmin) {
  ARITH(MIN, SIGNED)
}

def_EHelper(vmaxu) {
  ARITH(MAXU, UNSIGNED)
}

def_EHelper(vmax) {
  ARITH(MAX, SIGNED)
}

def_EHelper(vand) {
  ARITH(AND, SIGNED)
}

def_EHelper(vor) {
  ARITH(OR, SIGNED)
}

def_EHelper(vxor) {
  ARITH(XOR, SIGNED)
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
  ARITH(ADC, SIGNED)
}

def_EHelper(vmadc) {
  ARITH_MASK(MADC, SIGNED)
}

def_EHelper(vsbc) {
  ARITH(SBC, SIGNED)
}

def_EHelper(vmsbc) {
  ARITH_MASK(MSBC, SIGNED)
}

def_EHelper(vmerge) {
  if (s->vm == 1) {
    // when vm is 1, it's a vmv instruction
    if (id_src2->reg != 0) {
      // The first operand specifier (vs2) must contain v0
      longjmp_exception(EX_II);
    }
  }
  ARITH(MERGE, SIGNED)
}

def_EHelper(vmseq) {
  ARITH_MASK(MSEQ, SIGNED)
}

def_EHelper(vmsne) {
  ARITH_MASK(MSNE, SIGNED)
}

def_EHelper(vmsltu) {
  Assert(s->src_vmode != SRC_VI, "vmsltu not supprt SRC_VI\n");
  ARITH_MASK(MSLTU, UNSIGNED)
}

def_EHelper(vmslt) {
  Assert(s->src_vmode != SRC_VI, "vmslt not supprt SRC_VI\n");
  ARITH_MASK(MSLT, SIGNED)
}

def_EHelper(vmsleu) {
  ARITH_MASK(MSLEU, UNSIGNED)
}

def_EHelper(vmsle) {
  ARITH_MASK(MSLE, SIGNED);
}

def_EHelper(vmsgtu) {
  Assert(s->src_vmode != SRC_VV, "vmsgtu not support SRC_VV\n");
  ARITH_MASK(MSGTU, UNSIGNED)
}

def_EHelper(vmsgt) {
  Assert(s->src_vmode != SRC_VV, "vmsgt not support SRC_VV\n");
  ARITH_MASK(MSGT, SIGNED)
}

def_EHelper(vsaddu) {
  ARITH(SADDU, UNSIGNED)
}

def_EHelper(vsadd) {
  ARITH(SADD, SIGNED)
}

def_EHelper(vssubu) {
  ARITH(SSUBU, UNSIGNED)
}

def_EHelper(vssub) {
  ARITH(SSUB, SIGNED)
}

def_EHelper(vaadd) {
  ARITH(AADD, SIGNED)
}

def_EHelper(vaaddu) {
  ARITH(AADD, UNSIGNED)
}

def_EHelper(vsll) {
  ARITH(SLL, UNSIGNED)
}

def_EHelper(vasub) {
  ARITH(ASUB, SIGNED)
}

def_EHelper(vasubu) {
  ARITH(ASUB, UNSIGNED)
}

def_EHelper(vsmul) {
  ARITH(SMUL, SIGNED)
}

def_EHelper(vsrl) {
  ARITH(SRL, UNSIGNED)
}

def_EHelper(vsra) {
  ARITH(SRA, UNSIGNED)
}

def_EHelper(vssrl) {
  ARITH(SSRL, UNSIGNED)
}

def_EHelper(vssra) {
  ARITH(SSRA, SIGNED)
}

def_EHelper(vnsrl) {
  ARITH_NARROW(SRL, UNSIGNED, 1)
}

def_EHelper(vnsra) {
  ARITH_NARROW(SRA, UNSIGNED, 1)
}

def_EHelper(vnclipu) {
  ARITH_NARROW(NCLIPU, UNSIGNED, 1)
}

def_EHelper(vnclip) {
  ARITH_NARROW(NCLIP, SIGNED, 1)
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
  check_vstart_exception(s);
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
  vp_set_dirty();
}

def_EHelper(vmvxs) {
  require_vector(true);
  if (s->vm == 0) {
    longjmp_exception(EX_II);
  }
  check_vstart_exception(s);
  get_vreg(id_src2->reg, 0, s0, vtype->vsew, vtype->vlmul, 1, 0);
  rtl_sext(s, s0, s0, 8);
  rtl_sr(s, id_dest->reg, s0, 8);
  vstart->val = 0;
}

def_EHelper(vmvnr) {
  require_vector(true);
  check_vstart_exception(s);

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
  vp_set_dirty();
}

def_EHelper(vpopc) {
  require_vector(true);
  check_vstart_exception(s);
  
  rtl_li(s, s1, 0);
  for(word_t idx = vstart->val; idx < vl->val; idx ++) {
        // mask
    rtlreg_t mask = get_mask(0, idx);
    if(s->vm == 0 && mask == 0)
      continue;

    *s0 = get_mask(id_src2->reg, idx);
    *s0 &= 1;

    if(*s0 == 1)
      rtl_addi(s, s1, s1, 1);
  }
  rtl_sr(s, id_dest->reg, s1, 4);
  vstart->val = 0;
}

def_EHelper(vfirst) {
  require_vector(true);
  check_vstart_exception(s);

  int pos = -1;
  for(word_t idx = vstart->val; idx < vl->val; idx ++) {
    rtlreg_t mask = get_mask(0, idx);
    if (s->vm || mask) {
      *s0 = get_mask(id_src2->reg, idx);
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
  check_vstart_exception(s);

  if (vl->val != 0) {
    // when vl = 0, do nothing
    bool first_one = false;
    for(word_t idx = vstart->val; idx < vl->val; idx ++) {
      rtlreg_t mask = get_mask(0, idx);
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

      *s0 = get_mask(id_src2->reg, idx);
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
  vp_set_dirty();
}

def_EHelper(vmsof) {
  require_vector(true);
  require_vm(s);
  if (id_dest->reg == id_src2->reg) {
    longjmp_exception(EX_II);
  }
  check_vstart_exception(s);

  if (vl->val != 0) {
    // when vl = 0, do nothing
    bool first_one = false;
    for(word_t idx = vstart->val; idx < vl->val; idx ++) {
      rtlreg_t mask = get_mask(0, idx);
      if(s->vm == 0 && mask == 0) {
        if (RVV_AGNOSTIC) {
          if (vtype->vma) {
            set_mask(id_dest->reg, idx, 1, vtype->vsew, vtype->vlmul);
          }
        }
        continue;
      }

      *s0 = get_mask(id_src2->reg, idx);
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
  vp_set_dirty();
}

def_EHelper(vmsif) {
  require_vector(true);
  require_vm(s);
  if (id_dest->reg == id_src2->reg) {
    longjmp_exception(EX_II);
  }
  check_vstart_exception(s);

  if (vl->val != 0) {
    // when vl = 0, do nothing
    bool first_one = false;
    for(word_t idx = vstart->val; idx < vl->val; idx ++) {
      rtlreg_t mask = get_mask(0, idx);
      if(s->vm == 0 && mask == 0) {
        if (RVV_AGNOSTIC) {
          if (vtype->vma) {
            set_mask(id_dest->reg, idx, 1, vtype->vsew, vtype->vlmul);
          }
        }
        continue;
      }

      *s0 = get_mask(id_src2->reg, idx);
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
  vp_set_dirty();
}

def_EHelper(viota) {
  require_vector(true);
  require_vm(s);
  double vflmul = compute_vflmul();
  require_aligned(id_dest->reg, vflmul);
  require_noover(id_dest->reg, vflmul, id_src2->reg, 1);

  check_vstart_exception(s);
  if(!check_vstart_ignore(s)) {
    rtl_li(s, s1, 0);
    for(word_t idx = vstart->val; idx < vl->val; idx ++) {
      rtlreg_t mask = get_mask(0, idx);
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

      *s0 = get_mask(id_src2->reg, idx);
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
  vp_set_dirty();
}

def_EHelper(vid) {
  require_vector(true);
  require_vm(s);
  if (id_src2->reg != 0) {
    longjmp_exception(EX_II);
  }
  double vflmul = compute_vflmul();
  require_aligned(id_dest->reg, vflmul);

  check_vstart_exception(s);
  if(!check_vstart_ignore(s)) {
    for(word_t idx = 0; idx < vl->val; idx ++) {
      // mask
      rtlreg_t mask = get_mask(0, idx);
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
  vp_set_dirty();
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
  ARITH_NARROW(VEXT, UNSIGNED, -3);
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
  ARITH_NARROW(VEXT, SIGNED, -3);
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
  ARITH_NARROW(VEXT, UNSIGNED, -2);
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
  ARITH_NARROW(VEXT, SIGNED, -2);
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
  ARITH_NARROW(VEXT, UNSIGNED, -1);
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
  ARITH_NARROW(VEXT, SIGNED, -1);
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
  check_vstart_exception(s);
  if(!check_vstart_ignore(s)) {

    rtl_li(s, s1, 0);
    for(word_t idx = vstart->val; idx < vl->val; idx ++) {
      rtlreg_t mask = get_mask(id_src1->reg, idx);

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
  vp_set_dirty();
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
  ARITH(DIVU, UNSIGNED)
}

def_EHelper(vdiv) {
  Assert(s->src_vmode != SRC_VI, "vdiv does not support SRC_VI\n");
  ARITH(DIV, SIGNED)
}

def_EHelper(vremu) {
  Assert(s->src_vmode != SRC_VI, "vremu does not support SRC_VI\n");
  ARITH(REMU, UNSIGNED)
}

def_EHelper(vrem) {
  Assert(s->src_vmode != SRC_VI, "vrem does not support SRC_VI\n");
  ARITH(REM, SIGNED)
}

def_EHelper(vmulhu) {
  Assert(s->src_vmode != SRC_VI, "vmulhu does not support SRC_VI\n");
  ARITH(MULHU, UNSIGNED)
}

def_EHelper(vmul) {
  Assert(s->src_vmode != SRC_VI, "vmul does not support SRC_VI\n");
  ARITH(MUL, SIGNED)
}

def_EHelper(vmulhsu) {
  Assert(s->src_vmode != SRC_VI, "vmulhsu does not support SRC_VI\n");
  ARITH(MULHSU, UNSIGNED)
}

def_EHelper(vmulh) {
  Assert(s->src_vmode != SRC_VI, "vmulh does not support SRC_VI\n");
  ARITH(MULH, SIGNED)
}

def_EHelper(vmadd) {
  ARITH(MADD, SIGNED)
}

def_EHelper(vnmsub) {
  ARITH(NMSUB, SIGNED)
}

def_EHelper(vmacc) {
  ARITH(MACC, SIGNED)
}

def_EHelper(vnmsac) {
  ARITH(NMSAC, SIGNED)
}

def_EHelper(vwaddu) {
  ARITH_WIDE(ADD, UNSIGNED)
}

def_EHelper(vwadd) {
  ARITH_WIDE(ADD, SIGNED)
}

def_EHelper(vwsubu) {
  ARITH_WIDE(SUB, UNSIGNED)
}

def_EHelper(vwsub) {
  ARITH_WIDE(SUB, SIGNED)
}

def_EHelper(vwaddu_w) {
  arithmetic_instr(ADD, UNSIGNED, 1, 1, 0, s);
}

def_EHelper(vwadd_w) {
  arithmetic_instr(ADD, SIGNED, 1, 1, 0, s);
}

def_EHelper(vwsubu_w) {
  arithmetic_instr(SUB, UNSIGNED, 1, 1, 0, s);
}

def_EHelper(vwsub_w) {
  arithmetic_instr(SUB, SIGNED, 1, 1, 0, s);
}

def_EHelper(vwmulu) {
  ARITH_WIDE(MUL, UNSIGNED)
}

def_EHelper(vwmulsu) {
  ARITH_WIDE(MULSU, UNSIGNED)
}

def_EHelper(vwmul) {
  ARITH_WIDE(MUL, SIGNED)
}

def_EHelper(vwmaccu) {
  ARITH_WIDE(MACC, UNSIGNED)
}

def_EHelper(vwmacc) {
  ARITH_WIDE(MACC, SIGNED)
}

def_EHelper(vwmaccsu) {
  ARITH_WIDE(MACCSU, UNSIGNED)
}

def_EHelper(vwmaccus) {
  ARITH_WIDE(MACCUS, UNSIGNED)
}

def_EHelper(vfadd) {
  FLOAT_ARITH(FADD, UNSIGNED)
}

def_EHelper(vfredusum) {
#ifdef CONFIG_DIFFTEST
  FREDUCTION(FREDUSUM)    // use ordered reduction
#else
  float_reduction_computing(s);   // when NEMU is ref, use unordered reduction which is same as XiangShan
#endif
}

def_EHelper(vfsub) {
  FLOAT_ARITH(FSUB, UNSIGNED)
}

def_EHelper(vfredosum) {
  FREDUCTION(FREDOSUM)
}

def_EHelper(vfmin) {
  FLOAT_ARITH(FMIN, UNSIGNED)
}

def_EHelper(vfredmin) {
  FREDUCTION(FREDMIN)
}

def_EHelper(vfmax) {
  FLOAT_ARITH(FMAX, UNSIGNED)
}

def_EHelper(vfredmax) {
  FREDUCTION(FREDMAX)
}

def_EHelper(vfsgnj) {
  FLOAT_ARITH(FSGNJ, UNSIGNED)
}

def_EHelper(vfsgnjn) {
  FLOAT_ARITH(FSGNJN, UNSIGNED)
}

def_EHelper(vfsgnjx) {
  FLOAT_ARITH(FSGNJX, UNSIGNED)
}

def_EHelper(vfslide1up) {
  vector_slide_check(s, true);
  FLOAT_ARITH_NOCHECK(FSLIDE1UP)
}

def_EHelper(vfslide1down) {
  vector_slide_check(s, false);
  FLOAT_ARITH_NOCHECK(FSLIDE1DOWN)
}

def_EHelper(vfmvfs) {
  require_float();
  require_vector(true);
  check_vstart_exception(s);
  uint32_t rm = isa_fp_get_frm();
  isa_fp_rm_check(rm);
  if (s->vm == 0) {
    longjmp_exception(EX_II);
  }
  if (vtype->vsew == 0) {
    Loge("fp8 not supported");
    longjmp_exception(EX_II);
  }
#ifndef CONFIG_RV_ZVFH
  else if (vtype->vsew == 1) {
    Loge("ZVFH not supported");
    longjmp_exception(EX_II);
  }
#endif
  get_vreg(id_src2->reg, 0, s0, vtype->vsew, vtype->vlmul, 1, 0);
  if (vtype->vsew < 3) {
      *s0 = *s0 | (UINT64_MAX << (8 << vtype->vsew));
  }
  rtl_fsr(s, &fpreg_l(id_dest->reg), s0, FPCALL_W64);
  vstart->val = 0;
}

def_EHelper(vfmvsf) {
  require_float();
  require_vector(true);
  check_vstart_exception(s);
  uint32_t rm = isa_fp_get_frm();
  isa_fp_rm_check(rm);
  if (s->vm == 0) {
    longjmp_exception(EX_II);
  }
  if (vtype->vsew == 0) {
    Loge("fp8 not supported");
    longjmp_exception(EX_II);
  }
#ifndef CONFIG_RV_ZVFH
  else if (vtype->vsew == 1) {
    Loge("ZVFH not supported");
    longjmp_exception(EX_II);
  }
#endif
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
  vp_set_dirty();
  vstart->val = 0;
}

def_EHelper(vfcvt_xufv) {
  FLOAT_ARITH(FCVT_XUF, UNSIGNED)
}

def_EHelper(vfcvt_xfv) {
  FLOAT_ARITH(FCVT_XF, UNSIGNED)
}

def_EHelper(vfcvt_fxuv) {
  FLOAT_ARITH(FCVT_FXU, UNSIGNED)
}

def_EHelper(vfcvt_fxv) {
  FLOAT_ARITH(FCVT_FX, SIGNED)
}

def_EHelper(vfcvt_rtz_xufv) {
  FLOAT_ARITH(FCVT_RTZ_XUF, UNSIGNED)
}

def_EHelper(vfcvt_rtz_xfv) {
  FLOAT_ARITH(FCVT_RTZ_XF, UNSIGNED)
}

def_EHelper(vfwcvt_xufv) {
  FLOAT_ARITH_DWIDE(FWCVT_XUF, UNSIGNED)
}

def_EHelper(vfwcvt_xfv) {
  FLOAT_ARITH_DWIDE(FWCVT_XF, UNSIGNED)
}

def_EHelper(vfwcvt_fxuv) {
  FLOAT_ARITH_DWIDE(FWCVT_FXU, UNSIGNED)
}

def_EHelper(vfwcvt_fxv) {
  FLOAT_ARITH_DWIDE(FWCVT_FX, SIGNED)
}

def_EHelper(vfwcvt_ffv) {
  FLOAT_ARITH_DWIDE(FWCVT_FF, UNSIGNED)
}

def_EHelper(vfwcvt_rtz_xufv) {
  FLOAT_ARITH_DWIDE(FWCVT_RTZ_XUF, UNSIGNED)
}

def_EHelper(vfwcvt_rtz_xfv) {
  FLOAT_ARITH_DWIDE(FWCVT_RTZ_XF, UNSIGNED)
}

def_EHelper(vfncvt_xufw) {
  FLOAT_ARITH_DNARROW(FNCVT_XUF, UNSIGNED)
}

def_EHelper(vfncvt_xfw) {
  FLOAT_ARITH_DNARROW(FNCVT_XF, UNSIGNED)
}

def_EHelper(vfncvt_fxuw) {
  FLOAT_ARITH_DNARROW(FNCVT_FXU, UNSIGNED)
}

def_EHelper(vfncvt_fxw) {
  FLOAT_ARITH_DNARROW(FNCVT_FX, SIGNED)
}

def_EHelper(vfncvt_ffw) {
  FLOAT_ARITH_DNARROW(FNCVT_FF, UNSIGNED)
}

def_EHelper(vfncvt_rod_ffw) {
  FLOAT_ARITH_DNARROW(FNCVT_ROD_FF, UNSIGNED)
}

def_EHelper(vfncvt_rtz_xufw) {
  FLOAT_ARITH_DNARROW(FNCVT_RTZ_XUF, UNSIGNED)
}

def_EHelper(vfncvt_rtz_xfw) {
  FLOAT_ARITH_DNARROW(FNCVT_RTZ_XF, UNSIGNED)
}

def_EHelper(vfsqrt_v) {
  FLOAT_ARITH(FSQRT, UNSIGNED)
}

def_EHelper(vfrsqrt7_v) {
  FLOAT_ARITH(FRSQRT7, UNSIGNED)
}

def_EHelper(vfrec7_v) {
  FLOAT_ARITH(FREC7, UNSIGNED)
}

def_EHelper(vfclass_v) {
  FLOAT_ARITH(FCLASS, UNSIGNED)
}

def_EHelper(vfmerge) {
  require_float();
  require_vector(true);
  if (s->vm == 1) {
    // when vm is 1, it's a vfmv instruction
    if (id_src2->reg != 0) {
      // The first operand specifier (vs2) must contain v0
      longjmp_exception(EX_II);
    }
  }
  FLOAT_ARITH(FMERGE, UNSIGNED)
}

def_EHelper(vmfeq) {
  FLOAT_ARITH_MASK(MFEQ)
}

def_EHelper(vmfle) {
  FLOAT_ARITH_MASK(MFLE)
}

def_EHelper(vmflt) {
  FLOAT_ARITH_MASK(MFLT)
}

def_EHelper(vmfne) {
  FLOAT_ARITH_MASK(MFNE)
}

def_EHelper(vmfgt) {
  FLOAT_ARITH_MASK(MFGT)
}

def_EHelper(vmfge) {
  FLOAT_ARITH_MASK(MFGE)
}

def_EHelper(vfdiv) {
  FLOAT_ARITH(FDIV, UNSIGNED)
}

def_EHelper(vfrdiv) {
  FLOAT_ARITH(FRDIV, UNSIGNED)
}

def_EHelper(vfmul) {
  FLOAT_ARITH(FMUL, UNSIGNED)
}

def_EHelper(vfrsub) {
  FLOAT_ARITH(FRSUB, UNSIGNED)
}

def_EHelper(vfmadd) {
  FLOAT_ARITH(FMADD, UNSIGNED)
}

def_EHelper(vfnmadd) {
  FLOAT_ARITH(FNMADD, UNSIGNED)
}

def_EHelper(vfmsub) {
  FLOAT_ARITH(FMSUB, UNSIGNED)
}

def_EHelper(vfnmsub) {
  FLOAT_ARITH(FNMSUB, UNSIGNED)
}

def_EHelper(vfmacc) {
  FLOAT_ARITH(FMACC, UNSIGNED)
}

def_EHelper(vfnmacc) {
  FLOAT_ARITH(FNMACC, UNSIGNED)
}

def_EHelper(vfmsac) {
  FLOAT_ARITH(FMSAC, UNSIGNED)
}

def_EHelper(vfnmsac) {
  FLOAT_ARITH(FNMSAC, UNSIGNED)
}

def_EHelper(vfwadd) {
  FLOAT_ARITH_SDWIDE(FADD)
}

def_EHelper(vfwredusum) {
  FWREDUCTION(FREDUSUM)
}

def_EHelper(vfwsub) {
  FLOAT_ARITH_SDWIDE(FSUB)
}

def_EHelper(vfwredosum) {
  FWREDUCTION(FREDOSUM)
}

def_EHelper(vfwadd_w) {
  FLOAT_ARITH_SWIDE(FADD)
}

def_EHelper(vfwsub_w) {
  FLOAT_ARITH_SWIDE(FSUB)
}

def_EHelper(vfwmul) {
  FLOAT_ARITH_SDWIDE(FMUL)
}

def_EHelper(vfwmacc) {
  FLOAT_ARITH_SDWIDE(FMACC)
}

def_EHelper(vfwnmacc) {
  FLOAT_ARITH_SDWIDE(FNMACC)
}

def_EHelper(vfwmsac) {
  FLOAT_ARITH_SDWIDE(FMSAC)
}

def_EHelper(vfwnmsac) {
  FLOAT_ARITH_SDWIDE(FNMSAC)
}

def_EHelper(vandn) {
  ARITH(ANDN, SIGNED)
}

def_EHelper(vbrev_v) {
  ARITH(BREV_V, UNSIGNED)
}

def_EHelper(vbrev8_v) {
  ARITH(BREV8_V, UNSIGNED)
}

def_EHelper(vrev8_v) {
  ARITH(REV8_V, UNSIGNED)
}

def_EHelper(vclz_v) {
  ARITH(CLZ_V, UNSIGNED)
}

def_EHelper(vctz_v) {
  ARITH(CTZ_V, UNSIGNED)
}

def_EHelper(vcpop_v) {
  ARITH(CPOP_V, UNSIGNED)
}

def_EHelper(vrol) {
  ARITH(ROL, UNSIGNED)
}

def_EHelper(vror) {
  ARITH(ROR, UNSIGNED)
}

def_EHelper(vwsll) {
  ARITH_WIDE(SLL, UNSIGNED)
}

#endif // CONFIG_RVV