#include <common.h>
#ifdef CONFIG_RVV_010

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
  longjmp_raise_intr(EX_II);
}

def_EHelper(vmin) {
  longjmp_raise_intr(EX_II);
}

def_EHelper(vmaxu) {
  longjmp_raise_intr(EX_II);
}

def_EHelper(vmax) {
  longjmp_raise_intr(EX_II);
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
  longjmp_raise_intr(EX_II);
}

def_EHelper(vslideup) {
  longjmp_raise_intr(EX_II);
}

def_EHelper(vslidedown) {
  longjmp_raise_intr(EX_II);
}

def_EHelper(vadc) {
  longjmp_raise_intr(EX_II);
}

def_EHelper(vmadc) {
  longjmp_raise_intr(EX_II);
}

def_EHelper(vsbc) {
  longjmp_raise_intr(EX_II);
}

def_EHelper(vmsbc) {
  longjmp_raise_intr(EX_II);
}

def_EHelper(vmerge) {
  ARTHI(MERGE, SIGNED)
  // print_asm_template3(vmerge);
  // longjmp_raise_intr(EX_II);
}

def_EHelper(vmseq) {
  ARTHI_COMP(MSEQ, SIGNED)
  // print_asm_template3(vmseq);
  // longjmp_raise_intr(EX_II);
}

def_EHelper(vmsne) {
  ARTHI_COMP(MSNE, SIGNED)
  // print_asm_template3(vmsne);
  // longjmp_raise_intr(EX_II);
}

def_EHelper(vmsltu) {
  Assert(s->src_vmode != SRC_VI, "vmsltu not supprt SRC_VI\n");
  ARTHI_COMP(MSLTU, UNSIGNED)
  // print_asm_template3(vmsltu);
  // longjmp_raise_intr(EX_II);
}

def_EHelper(vmslt) {
  Assert(s->src_vmode != SRC_VI, "vmslt not supprt SRC_VI\n");
  ARTHI_COMP(MSLT, SIGNED)
  // print_asm_template3(vmslt);
  // longjmp_raise_intr(EX_II);
}

def_EHelper(vmsleu) {
  ARTHI_COMP(MSLEU, UNSIGNED)
  // print_asm_template3(vmsleu);
  // longjmp_raise_intr(EX_II);
}

def_EHelper(vmsle) {
  ARTHI_COMP(MSLE, SIGNED);
  // print_asm_template3(vmsle);
  // longjmp_raise_intr(EX_II);
}

def_EHelper(vmsgtu) {
  Assert(s->src_vmode != SRC_VV, "vmsgtu not support SRC_VV\n");
  ARTHI_COMP(MSGTU, UNSIGNED)
  // print_asm_template3(vmsgtu);
  // longjmp_raise_intr(EX_II);
}

def_EHelper(vmsgt) {
  Assert(s->src_vmode != SRC_VV, "vmsgt not support SRC_VV\n");
  ARTHI_COMP(MSGT, SIGNED)
  // print_asm_template3(vmsgt);
  // longjmp_raise_intr(EX_II);
}

def_EHelper(vsaddu) {
  longjmp_raise_intr(EX_II);
}

def_EHelper(vsadd) {
  longjmp_raise_intr(EX_II);
}

def_EHelper(vssubu) {
  longjmp_raise_intr(EX_II);
}

def_EHelper(vssub) {
  longjmp_raise_intr(EX_II);
}

def_EHelper(vaadd) {
  longjmp_raise_intr(EX_II);
}

def_EHelper(vsll) {
  ARTHI(SLL, UNSIGNED)
  // print_asm_template3(vsll);
  // longjmp_raise_intr(EX_II);
}

def_EHelper(vasub) {
  longjmp_raise_intr(EX_II);
}

def_EHelper(vsmul) {
  longjmp_raise_intr(EX_II);
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
  longjmp_raise_intr(EX_II);
}

def_EHelper(vssra) {
  longjmp_raise_intr(EX_II);
}

def_EHelper(vnsrl) {
  longjmp_raise_intr(EX_II);
}

def_EHelper(vnsra) {
  longjmp_raise_intr(EX_II);
}

def_EHelper(vnclipu) {
  longjmp_raise_intr(EX_II);
}

def_EHelper(vnclip) {
  longjmp_raise_intr(EX_II);
}

def_EHelper(vwredsumu) {
  longjmp_raise_intr(EX_II);
}

def_EHelper(vwredsum) {
  longjmp_raise_intr(EX_II);
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
  REDInstr(REDSUM, SIGNED);
  // longjmp_raise_intr(EX_II);
}

def_EHelper(vredand) {
  REDInstr(REDAND, UNSIGNED);
  // longjmp_raise_intr(EX_II);
}

def_EHelper(vredor) {
  REDInstr(REDOR, UNSIGNED);
  // longjmp_raise_intr(EX_II);
}

def_EHelper(vredxor) {
  REDInstr(REDXOR, UNSIGNED);
  // longjmp_raise_intr(EX_II);
}

def_EHelper(vredminu) {
  longjmp_raise_intr(EX_II);
}

def_EHelper(vredmin) {
  longjmp_raise_intr(EX_II);
}

def_EHelper(vredmaxu) {
  longjmp_raise_intr(EX_II);
}

def_EHelper(vredmax) {
  longjmp_raise_intr(EX_II);
}




def_EHelper(vmpopc) {
  // longjmp_raise_intr(EX_II);
  if(vstart->val != 0)
    longjmp_raise_intr(EX_II);
  
  int vlmax = ((VLEN >> 3) >> vtype->vsew) << vtype->vlmul;
  rtl_li(s, s1, 0);
  for(int idx = 0; idx < vlmax; idx ++) {
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

def_EHelper(vmfirst) {
  // longjmp_raise_intr(EX_II);
  if(vstart->val != 0)
    longjmp_raise_intr(EX_II);
  
  int vlmax = ((VLEN >> 3) >> vtype->vsew) << vtype->vlmul;
  int idx;
  for(idx = 0; idx < vlmax; idx ++) {
    *s0 = get_mask(id_src2->reg, idx, vtype->vsew, vtype->vlmul);
    *s0 &= 1;
    if(*s0 == 1) break;
  }
  if(idx < vlmax)
    rtl_li(s, s1, idx);  
  else
    rtl_li(s, s1, -1);
  rtl_sr(s, id_dest->reg, s1, 4);
}

def_EHelper(vmunaryo) {
  longjmp_raise_intr(EX_II);
}

def_EHelper(vcompress) {
  longjmp_raise_intr(EX_II);
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
  longjmp_raise_intr(EX_II);
}

def_EHelper(vwadd) {
  longjmp_raise_intr(EX_II);
}

def_EHelper(vwsubu) {
  longjmp_raise_intr(EX_II);
}

def_EHelper(vwsub) {
  longjmp_raise_intr(EX_II);
}

def_EHelper(vwaddu_w) {
  longjmp_raise_intr(EX_II);
}

def_EHelper(vwadd_w) {
  longjmp_raise_intr(EX_II);
}

def_EHelper(vwsubu_w) {
  longjmp_raise_intr(EX_II);
}

def_EHelper(vwsub_w) {
  longjmp_raise_intr(EX_II);
}

def_EHelper(vwmulu) {
  longjmp_raise_intr(EX_II);
}

def_EHelper(vwmulsu) {
  longjmp_raise_intr(EX_II);
}

def_EHelper(vwmul) {
  longjmp_raise_intr(EX_II);
}

def_EHelper(vwmaccu) {
  longjmp_raise_intr(EX_II);
}

def_EHelper(vwnmacc) {
  longjmp_raise_intr(EX_II);
}

def_EHelper(vwmaccsu) {
  longjmp_raise_intr(EX_II);
}

def_EHelper(vwmaccus) {
  longjmp_raise_intr(EX_II);
}


#endif // CONFIG_RVV_010