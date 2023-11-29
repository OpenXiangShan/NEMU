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

// #define PERMInstr(opcode) permutation_instr(opcode, pc);
// static void permutation_instr(int opcode, vaddr_t* pc) {
//   // only move/ext here, no slide
//   switch (opcode) {
//     case EXT_X_V : 
//   }
// }

def_EHelper(vadd) {
  ARTHI(ADD, SIGNED)
  // print_asm_template3(vadd);
}

def_EHelper(vsub) {
  Assert(s->src_vmode != SRC_VI, "vsub.vi not supported\n");
  ARTHI(SUB, SIGNED)
  // print_asm_template3(vsub);
}

def_EHelper(vrsub) {
  Assert(s->src_vmode != SRC_VV, "vrsub.vv not supported\n");
  ARTHI(RSUB, SIGNED)
  // print_asm_template3(vrsub);
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
  // print_asm_template3(vand);
  // longjmp_raise_intr(EX_II);
}

def_EHelper(vor) {
  ARTHI(OR, SIGNED)
  // print_asm_template3(vor);
  // longjmp_raise_intr(EX_II);
}

def_EHelper(vxor) {
  ARTHI(XOR, SIGNED)
  // print_asm_template3(vxor);
  // longjmp_raise_intr(EX_II);
}

def_EHelper(vrgather) {
  ARTHI(RGATHER, UNSIGNED)
}

def_EHelper(vrgatherei16) {
  ARTHI(RGATHEREI16, UNSIGNED)
}

def_EHelper(vslideup) {
  ARTHI(SLIDEUP, UNSIGNED)
}

def_EHelper(vslidedown) {
  ARTHI(SLIDEDOWN, UNSIGNED)
}

def_EHelper(vslide1up) {
  ARTHI(SLIDE1UP, UNSIGNED)
}

def_EHelper(vslide1down) {
  ARTHI(SLIDE1DOWN, UNSIGNED)
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
  ARTHI(MERGE, SIGNED)
  // print_asm_template3(vmerge);
  // longjmp_raise_intr(EX_II);
}

def_EHelper(vmseq) {
  ARTHI_MASK(MSEQ, SIGNED)
  // print_asm_template3(vmseq);
  // longjmp_raise_intr(EX_II);
}

def_EHelper(vmsne) {
  ARTHI_MASK(MSNE, SIGNED)
  // print_asm_template3(vmsne);
  // longjmp_raise_intr(EX_II);
}

def_EHelper(vmsltu) {
  Assert(s->src_vmode != SRC_VI, "vmsltu not supprt SRC_VI\n");
  ARTHI_MASK(MSLTU, UNSIGNED)
  // print_asm_template3(vmsltu);
  // longjmp_raise_intr(EX_II);
}

def_EHelper(vmslt) {
  Assert(s->src_vmode != SRC_VI, "vmslt not supprt SRC_VI\n");
  ARTHI_MASK(MSLT, SIGNED)
  // print_asm_template3(vmslt);
  // longjmp_raise_intr(EX_II);
}

def_EHelper(vmsleu) {
  ARTHI_MASK(MSLEU, UNSIGNED)
  // print_asm_template3(vmsleu);
  // longjmp_raise_intr(EX_II);
}

def_EHelper(vmsle) {
  ARTHI_MASK(MSLE, SIGNED);
  // print_asm_template3(vmsle);
  // longjmp_raise_intr(EX_II);
}

def_EHelper(vmsgtu) {
  Assert(s->src_vmode != SRC_VV, "vmsgtu not support SRC_VV\n");
  ARTHI_MASK(MSGTU, UNSIGNED)
  // print_asm_template3(vmsgtu);
  // longjmp_raise_intr(EX_II);
}

def_EHelper(vmsgt) {
  Assert(s->src_vmode != SRC_VV, "vmsgt not support SRC_VV\n");
  ARTHI_MASK(MSGT, SIGNED)
  // print_asm_template3(vmsgt);
  // longjmp_raise_intr(EX_II);
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
  // print_asm_template3(vsll);
  // longjmp_raise_intr(EX_II);
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
  // print_asm_template3(vsrl);
  //longjmp_raise_intr(EX_II);
}

def_EHelper(vsra) {
  ARTHI(SRA, UNSIGNED)
  // print_asm_template3(vsra);
  // longjmp_raise_intr(EX_II);
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
  longjmp_raise_intr(EX_II);
}

def_EHelper(vdot) {
  longjmp_raise_intr(EX_II);
}

def_EHelper(vwsmaccu) {
  longjmp_raise_intr(EX_II);
}

def_EHelper(vwsmacc) {
  longjmp_raise_intr(EX_II);
}

def_EHelper(vwsmaccsu) {
  longjmp_raise_intr(EX_II);
}

def_EHelper(vwsmaccus) {
  longjmp_raise_intr(EX_II);
}


//op-m
def_EHelper(vredsum) {
  REDUCTION(REDSUM, SIGNED);
  // longjmp_raise_intr(EX_II);
}

def_EHelper(vredand) {
  REDUCTION(REDAND, UNSIGNED);
  // longjmp_raise_intr(EX_II);
}

def_EHelper(vredor) {
  REDUCTION(REDOR, UNSIGNED);
  // longjmp_raise_intr(EX_II);
}

def_EHelper(vredxor) {
  REDUCTION(REDXOR, UNSIGNED);
  // longjmp_raise_intr(EX_II);
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

def_EHelper(vmvsx) {
  if (vstart->val < vl->val) {
    rtl_lr(s, &(id_src->val), id_src1->reg, 4);
    rtl_mv(s, s1, &id_src->val); 
    rtl_sext(s, s1, s1, 1 << vtype->vsew);
    set_vreg(id_dest->reg, 0, *s1, vtype->vsew, vtype->vlmul, 1);
    if (RVV_AGNOSTIC) {
      if(vtype->vta) {
        for (int idx = 8 << vtype->vsew; idx < VLEN; idx++) {
          set_mask(id_dest->reg, idx, 1, vtype->vsew, vtype->vlmul);
        }
      }
    }
  }
}

def_EHelper(vmvxs) {
    get_vreg(id_src2->reg, 0, s0, vtype->vsew, vtype->vlmul, 1, 1);
    rtl_sext(s, s0, s0, 8);
    rtl_sr(s, id_dest->reg, s0, 8);
}

def_EHelper(vmvnr) {
    rtl_li(s, s1, s->isa.instr.v_opv3.v_imm5);
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
}

def_EHelper(vpopc) {
  // longjmp_raise_intr(EX_II);
  if(vstart->val != 0)
    longjmp_raise_intr(EX_II);
  
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
}

def_EHelper(vfirst) {
  // longjmp_raise_intr(EX_II);
  if(vstart->val != 0)
    longjmp_raise_intr(EX_II);

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
}

def_EHelper(vmsbf) {
  if(vstart->val != 0)
    longjmp_raise_intr(EX_II);

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
      first_one = true;
    }

    if(first_one) {
      set_mask(id_dest->reg, idx, 0, vtype->vsew, vtype->vlmul);
    } else{
      set_mask(id_dest->reg, idx, 1, vtype->vsew, vtype->vlmul);
    }
  }
  // If there is no set bit in the active element of the source vector,
  // all active elements in the target are written to 1.
  if (!first_one) {
    for(int idx = vstart->val; idx < vl->val; idx ++) {
      set_mask(id_dest->reg, idx, 1, vtype->vsew, vtype->vlmul);
    }
  }
  if (RVV_AGNOSTIC) {
    for (int idx = vl->val; idx < VLEN; idx++) {
      set_mask(id_dest->reg, idx, 1, vtype->vsew, vtype->vlmul);
    }
  }
}

def_EHelper(vmsof) {
  if(vstart->val != 0)
    longjmp_raise_intr(EX_II);

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
}

def_EHelper(vmsif) {
  if(vstart->val != 0)
    longjmp_raise_intr(EX_II);

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
}

def_EHelper(viota) {
  if(vstart->val != 0)
    longjmp_raise_intr(EX_II);

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
      int vlmax = get_vlen_max(vtype->vsew, vtype->vlmul);
      for(int idx = vl->val; idx < vlmax; idx++) {
        *s1 = (uint64_t) -1;
        set_vreg(id_dest->reg, idx, *s1, vtype->vsew, vtype->vlmul, 1);
      }
    }
  }
}

def_EHelper(vid) {
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
      int vlmax = get_vlen_max(vtype->vsew, vtype->vlmul);
      for(int idx = vl->val; idx < vlmax; idx++) {
        *s1 = (uint64_t) -1;
        set_vreg(id_dest->reg, idx, *s1, vtype->vsew, vtype->vlmul, 1);
      }
    }
  }
}

def_EHelper(vzextvf8) {
  ARTHI_NARROW(VEXT, UNSIGNED, -3);
}

def_EHelper(vsextvf8) {
  ARTHI_NARROW(VEXT, SIGNED, -3);
}

def_EHelper(vzextvf4) {
  ARTHI_NARROW(VEXT, UNSIGNED, -2);
}

def_EHelper(vsextvf4) {
  ARTHI_NARROW(VEXT, SIGNED, -2);
}

def_EHelper(vzextvf2) {
  ARTHI_NARROW(VEXT, UNSIGNED, -1);
}

def_EHelper(vsextvf2) {
  ARTHI_NARROW(VEXT, SIGNED, -1);
}

def_EHelper(vcompress) {
  if(vstart->val != 0)
    longjmp_raise_intr(EX_II);

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
      int vlmax = get_vlen_max(vtype->vsew, vtype->vlmul);
      for(int idx = *s1; idx < vlmax; idx++) {
        *s1 = (uint64_t) -1;
        set_vreg(id_dest->reg, idx, *s1, vtype->vsew, vtype->vlmul, 1);
      }
    }
  }
}

def_EHelper(vmandnot) {
  MASKINSTR(MANDNOT)
  // print_asm_template3(vmandnot);
  // longjmp_raise_intr(EX_II);
}

def_EHelper(vmand) {
  MASKINSTR(MAND)
  // print_asm_template3(vmand);
  // longjmp_raise_intr(EX_II);
}

def_EHelper(vmor) {
  MASKINSTR(MOR)
  // print_asm_template3(vmor);
  // longjmp_raise_intr(EX_II);
}

def_EHelper(vmxor) {
  MASKINSTR(MXOR)
  // print_asm_template3(vmxor);
  // longjmp_raise_intr(EX_II);
}

def_EHelper(vmornot) {
  MASKINSTR(MORNOT)
  // print_asm_template3(vmornot);
  // longjmp_raise_intr(EX_II);
}

def_EHelper(vmnand) {
  MASKINSTR(MNAND)
  // print_asm_template3(vmnand);
  // longjmp_raise_intr(EX_II);
}

def_EHelper(vmnor) {
  MASKINSTR(MNOR)
  // print_asm_template3(vmnor);
  // longjmp_raise_intr(EX_II);
}

def_EHelper(vmxnor) {
  MASKINSTR(MXNOR);
  // print_asm_template3(vmnor);
  // longjmp_raise_intr(EX_II);
}

def_EHelper(vdivu) {
  Assert(s->src_vmode != SRC_VI, "vdivu does not support SRC_VI\n");
  ARTHI(DIVU, UNSIGNED)
  // print_asm_template3(vdivu);
  // longjmp_raise_intr(EX_II);
}

def_EHelper(vdiv) {
  Assert(s->src_vmode != SRC_VI, "vdiv does not support SRC_VI\n");
  ARTHI(DIV, SIGNED)
  // print_asm_template3(vdiv);
  // longjmp_raise_intr(EX_II);
}

def_EHelper(vremu) {
  Assert(s->src_vmode != SRC_VI, "vremu does not support SRC_VI\n");
  ARTHI(REMU, UNSIGNED)
  // print_asm_template3(vremu);
  // longjmp_raise_intr(EX_II);
}

def_EHelper(vrem) {
  Assert(s->src_vmode != SRC_VI, "vrem does not support SRC_VI\n");
  ARTHI(REM, SIGNED)
  // print_asm_template3(vrem);
  // longjmp_raise_intr(EX_II);
}

def_EHelper(vmulhu) {
  Assert(s->src_vmode != SRC_VI, "vmulhu does not support SRC_VI\n");
  ARTHI(MULHU, UNSIGNED)
  // print_asm_template3(vmulhu);
  // longjmp_raise_intr(EX_II);
}

def_EHelper(vmul) {
  Assert(s->src_vmode != SRC_VI, "vmul does not support SRC_VI\n");
  ARTHI(MUL, SIGNED)
  // print_asm_template3(vmul);
  // longjmp_raise_intr(EX_II);
}

def_EHelper(vmulhsu) {
  Assert(s->src_vmode != SRC_VI, "vmulhsu does not support SRC_VI\n");
  ARTHI(MULHSU, UNSIGNED)
  // print_asm_template3(vmulshu);
  // longjmp_raise_intr(EX_II);
}

def_EHelper(vmulh) {
  Assert(s->src_vmode != SRC_VI, "vmulh does not support SRC_VI\n");
  ARTHI(MULH, SIGNED)
  // print_asm_template3(vmulh);
  // longjmp_raise_intr(EX_II);
}

def_EHelper(vmadd) {
  ARTHI(MADD, SIGNED)
  // print_asm_template3(vmadd);
  // longjmp_raise_intr(EX_II);
}

def_EHelper(vnmsub) {
  ARTHI(NMSUB, SIGNED)
  // print_asm_template3(vnmsub);
  // longjmp_raise_intr(EX_II);
}

def_EHelper(vmacc) {
  ARTHI(MACC, SIGNED)
  // print_asm_template3(vmacc);
  // longjmp_raise_intr(EX_II);
}

def_EHelper(vnmsac) {
  ARTHI(NMSAC, SIGNED)
  // print_asm_template3(vmacc);
  // longjmp_raise_intr(EX_II);
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
  FREDUCTION(FREDUSUM)
  //float_reduction_computing(s);
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
  FLOAT_ARTHI(FSLIDE1UP, UNSIGNED)
}

def_EHelper(vfslide1down) {
  FLOAT_ARTHI(FSLIDE1DOWN, UNSIGNED)
}

def_EHelper(vfmvfs) {
  // vfmv.f.s performs its action even if vstart≥vl or vl=0
  if (vstart->val < vl->val || vl->val == 0) {
    get_vreg(id_src2->reg, 0, s0, vtype->vsew, vtype->vlmul, 1, 1);
    if (vtype->vsew < 3) {
        *s0 = *s0 | (UINT64_MAX << (8 << vtype->vsew));
    }
    rtl_mv(s, &fpreg_l(id_dest->reg), s0);
  }
}

def_EHelper(vfmvsf) {
  if (vstart->val < vl->val) {
    rtl_mv(s, s1, &fpreg_l(id_src1->reg)); // f[rs1]
    set_vreg(id_dest->reg, 0, *s1, vtype->vsew, vtype->vlmul, 1);
    if (RVV_AGNOSTIC) {
      if(vtype->vta) {
        for (int idx = 8 << vtype->vsew; idx < VLEN; idx++) {
          set_mask(id_dest->reg, idx, 1, vtype->vsew, vtype->vlmul);
        }
      }
    }
  }
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
  FLOAT_ARTHI_DWIDE(FWCVT_FXU, UNSIGNED)
}

def_EHelper(vfwcvt_fxv) {
  FLOAT_ARTHI_DWIDE(FWCVT_FX, SIGNED)
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
  FLOAT_ARTHI_DNARROW(FNCVT_XUF, UNSIGNED)
}

def_EHelper(vfncvt_xfw) {
  FLOAT_ARTHI_DNARROW(FNCVT_XF, UNSIGNED)
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
  FLOAT_ARTHI_DNARROW(FNCVT_RTZ_XUF, UNSIGNED)
}

def_EHelper(vfncvt_rtz_xfw) {
  FLOAT_ARTHI_DNARROW(FNCVT_RTZ_XF, UNSIGNED)
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

#endif // CONFIG_RVV