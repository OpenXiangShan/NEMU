#include "cpu/exec.h"
#include "vreg.h"
#include "../local-include/csr.h"
#include <stdio.h>
#include "../local-include/intr.h"
#include "../local-include/rtl.h"
#include <setjmp.h>

#define id_src (&s->src1)
#define id_src2 (&s->src2)
#define id_dest (&s->dest)


enum op_t {
  ADD, SUB, RSUB, MINU, MIN, MAXU, MAX, AND,
  OR, XOR, RGATHER, SLIDEUP, SLIDEDOWN, ADC, MADC, SBC,
  MSBC, MERGE, MSEQ, MSNE, MSLTU, MSLT, MSLEU, MSLE,
  MSGTU, MSGT, SADDU, SADD, SSUBU, SSUB, AADD, SLL,
  ASUB, SMUL, SRL, SRA, SSRL, SSRA, NSRL, NSRA,
  NCLIPU, NCLIP, WREDSUMU, WREDSUM, DOTU, DOT, WSMACCU, WSMACC,
  WSMAXXSU, WSMACCUS,

  REDSUM, REDAND, REDOR, REDXOR, REDMINU, REDMIN, REDMAXU, REDMAX,
  EXT_X_V, MV_S_X, SLIDE1UP, SLIDE1DOWN, MPOPC, VMFIRST, MUNARYO, COMPRESS,
  MANDNOT, MAND, MOR, MXOR, MORNOT, MNAND, MNOR, MXNOR,
  DIVU, DIV, REMU, REM, MULHU, MUL, MULHSU, MULH,
  MADD, NMSUB, MACC, NMSAC, WADDU, WADD, WSUBU, WSUB,
  WADDU_W, WADD_W, WSUBU_W, WSUB_W, WMULU, WMULSU, WMUL, WMACCU,
  WNMACC, WMACCSU, WMACCUS,
};

#define ARTHI(opcode, is_signed) arthimetic_instr(opcode, is_signed, 0, s);
#define ARTHI_COMP(opcode, is_signed) arthimetic_instr(opcode, is_signed, 1, s);
void arthimetic_instr(int opcode, int is_signed, int dest_reg, Decode *s) {
  int idx;
  for(idx = vstart->val; idx < vl->val; idx ++) {
    // mask
    rtlreg_t mask = get_mask(0, idx, vtype->vsew, vtype->vlmul);
    if(s->vm == 0) {
      // merge instr will exec no matter mask or not
      // masked and mask off exec will left dest unmodified.
      if(opcode != MERGE && mask==0) continue;
    } else if(opcode == MERGE) {
      mask = 1; // merge(mv) get the first operand (vs1, rs1, imm);
    }

    // operand - vs2
    get_vreg(id_src2->reg, idx, s0, vtype->vsew, vtype->vlmul, is_signed, 1);
     if(is_signed) rtl_sext(s, s0, s0, 1 << vtype->vsew);

    // operand - vs1 / rs1 / imm
    switch (s->src_vmode) {
      case SRC_VV : 
        get_vreg(id_src->reg, idx, s1, vtype->vsew, vtype->vlmul, is_signed, 1);
        if(is_signed) rtl_sext(s, s1, s1, 1 << vtype->vsew);
        break;
      case SRC_VS :   
        rtl_lr(s, &(id_src->val), id_src1->reg, 4);
        rtl_mv(s, s1, &id_src->val); 
        if(is_signed) rtl_sext(s, s1, s1, 1 << vtype->vsew);
        break;
      case SRC_VI :
        if(is_signed) rtl_li(s, s1, s->isa.instr.v_opv2.v_simm5);
        else          rtl_li(s, s1, s->isa.instr.v_opv3.v_imm5 );
        break;
    }

    // op
    switch (opcode) {
      case ADD : rtl_add(s, s1, s0, s1); break;
      case SUB : rtl_sub(s, s1, s0, s1); break;
      case RSUB: rtl_sub(s, s1, s1, s0); break;
      case AND : rtl_and(s, s1, s0, s1); break;
      case OR  : rtl_or(s, s1, s0, s1); break;
      case XOR : rtl_xor(s, s1, s0, s1); break;
      case SLL :
        rtl_andi(s, s1, s1, s->v_width*8-1); //low lg2(SEW) is valid
        //rtl_sext(s0, s0, 8 - (1 << vtype->vsew)); //sext first
        rtl_shl(s, s1, s0, s1); break;
      case SRL :
        rtl_andi(s, s1, s1, s->v_width*8-1); //low lg2(SEW)
        rtl_shr(s, s1, s0, s1); break;
      case SRA :
        rtl_andi(s, s1, s1, s->v_width*8-1); //low lg2(SEW)
        rtl_sext(s, s0, s0, s->v_width);
        rtl_sar(s, s1, s0, s1); break;
      case MULHU : 
        vs1 = (uint64_t)(((__uint128_t)(vs0) * (__uint128_t)(vs1))>>(s->v_width*8));
        break;
      case MUL : rtl_mulu_lo(s, s1, s0, s1); break;
      case MULHSU :
        rtl_sext(s, t0, s0, s->v_width);
        rtl_sari(s, t0, t0, s->v_width*8-1);
        rtl_and(s, t0, s1, t0);
        vs1 = (uint64_t)(((__uint128_t)(vs0) * (__uint128_t)(vs1))>>(s->v_width*8));
        rtl_sub(s, s1, s1, t0);
        break;
      case MULH :
        rtl_sext(s, s0, s0, s->v_width);
        rtl_sext(s, s1, s1, s->v_width);
        vs1 = (uint64_t)(((__int128_t)(sword_t)(vs0) * (__int128_t)(sword_t)(vs1))>>(s->v_width*8));
        break;
      case MACC : 
        rtl_mulu_lo(s, s1, s0, s1);
        get_vreg(id_dest->reg, idx, s0, vtype->vsew, vtype->vlmul, is_signed, 1);
        rtl_add(s, s1, s1, s0);
        break;
      case NMSAC :
        rtl_mulu_lo(s, s1, s0, s1);
        get_vreg(id_dest->reg, idx, s0, vtype->vsew, vtype->vlmul, is_signed, 1);
        rtl_sub(s, s1, s0, s1);
        break;
      case MADD :
        get_vreg(id_dest->reg, idx, s0, vtype->vsew, vtype->vlmul, is_signed, 1);
        rtl_mulu_lo(s, s1, s0, s1);
        get_vreg(id_src2->reg, idx, s0, vtype->vsew, vtype->vlmul, is_signed, 1);
        rtl_add(s, s1, s1, s0);
        break;
      case NMSUB :
        get_vreg(id_dest->reg, idx, s0, vtype->vsew, vtype->vlmul, is_signed, 1);
        rtl_mulu_lo(s, s1, s1, s0);
        get_vreg(id_src2->reg, idx, s0, vtype->vsew, vtype->vlmul, is_signed, 1);
        rtl_sub(s, s1, s0, s1);
        break;
      case DIVU :
        if(*s1 == 0) rtl_li(s, s1, ~0lu);
        else rtl_divu_q(s, s1, s0, s1);
        break;
      case DIV :
        rtl_sext(s, s0, s0, s->v_width);
        rtl_sext(s, s1, s1, s->v_width);
        if(*s1 == 0) rtl_li(s, s1, ~0lu);
        else if(*s0 == 0x8000000000000000LL && *s1 == -1) //may be error
          rtl_mv(s, s1, s0);
        else rtl_divs_q(s, s1, s0, s1);
        break;
      case REMU :
        if (*s1 == 0) rtl_mv(s, s1, s0);
        else rtl_divu_r(s, s1, s0, s1);
        break;
      case REM :
        rtl_sext(s, s0, s0, s->v_width);
        rtl_sext(s, s1, s1, s->v_width);
        if(*s1 == 0) rtl_mv(s, s1, s0);
        else if(*s1 == 0x8000000000000000LL && *s1 == -1) //may be error
          rtl_li(s, s1, 0);
        else rtl_divs_r(s, s1, s0, s1);
        break;
      case MERGE : rtl_mux(s, s1, &mask, s1, s0); break;
      case MSEQ  : rtl_setrelop(s, RELOP_EQ,  s1, s0, s1); break;
      case MSNE  : rtl_setrelop(s, RELOP_NE,  s1, s0, s1); break;
      case MSLTU : rtl_setrelop(s, RELOP_LTU, s1, s0, s1); break;
      case MSLT  : rtl_setrelop(s, RELOP_LT,  s1, s0, s1); break;
      case MSLEU : rtl_setrelop(s, RELOP_LEU, s1, s0, s1); break;
      case MSLE  : rtl_setrelop(s, RELOP_LE,  s1, s0, s1); break;
      case MSGTU : rtl_setrelop(s, RELOP_GTU, s1, s0, s1); break;
      case MSGT  : rtl_setrelop(s, RELOP_GT,  s1, s0, s1); break;
    }

    // store to vrf
    if(dest_reg == 1) 
      set_mask(id_dest->reg, idx, *s1, vtype->vsew, vtype->vlmul);
    else 
      set_vreg(id_dest->reg, idx, *s1, vtype->vsew, vtype->vlmul, 1);
  }

  // idx gt the vl need to be zeroed.
  // int vlmax = ((VLEN >> 3) >> vtype->vsew) << vtype->vlmul;
  // for(idx = vl->val; idx < vlmax; idx ++) {
  //   rtl_li(s1, 0);
  //   if(dest_reg == 1) 
  //     set_mask(id_dest->reg, idx, s1, vtype->vsew, vtype->vlmul);
  //   else 
  //     set_vreg(id_dest->reg, idx, s1, vtype->vsew, vtype->vlmul, 1);
  // }

  // TODO: the idx larger than vl need reset to zero.
  rtl_li(s, s0, 0);
  vcsr_write(IDXVSTART, s0);
}

#define MASKINSTR(opcode) mask_instr(opcode, s);
void mask_instr(int opcode, Decode *s) {
  int idx;
  for(idx = vstart->val; idx < vl->val; idx++) {
    // operand - vs2
    *s0 = get_mask(id_src2->reg, idx, vtype->vsew, vtype->vlmul); // unproper usage of s0
    *s0 &= 1; // only LSB

    // operand - vs1
    *s1 = get_mask(id_src->reg, idx, vtype->vsew, vtype->vlmul); // unproper usage of s1
    *s1 &= 1; // only LSB

    // op
    switch (opcode) {
      case MAND    : rtl_and(s, s1, s0, s1); break;
      case MNAND   : rtl_and(s, s1, s0, s1);
                     *s1 = !(*s1); break;
      case MANDNOT : *s1 = !(*s1); // unproper usage of not
                     rtl_and(s, s1, s0, s1); break;
      case MXOR    : rtl_xor(s, s1, s0, s1); break;
      case MOR     : rtl_or(s, s1, s0, s1); break;
      case MNOR    : rtl_or(s, s1, s0, s1);
                     *s1 = !(*s1); break;
      case MORNOT  : *s1 = !(*s1);
                     rtl_or(s, s1, s0, s1); break;
      case MXNOR   : rtl_xor(s, s1, s0, s1);
                     *s1 = !(*s1); break;
      default      : longjmp_raise_intr(EX_II);
    }
    // store to vrf
    *s1 &= 1; // make sure the LSB
    set_mask(id_dest->reg, idx, *s1, vtype->vsew, vtype->vlmul);
  }

  int vlmax = ((VLEN >> 3) >> vtype->vsew) << vtype->vlmul;
  rtl_li(s, s1, 0);
  for( idx = vl->val; idx < vlmax; idx++) {  
    set_mask(id_dest->reg, idx, vs1, vtype->vsew, vtype->vlmul);
  }
  vcsr_write(IDXVSTART, s1);
}

#define REDInstr(opcode, is_signed) reduction_instr(opcode, is_signed, s);
void reduction_instr(int opcode, int is_signed, Decode *s) {
  get_vreg(id_src->reg, 0, s1, vtype->vsew, vtype->vlmul, is_signed, 0);
  if(is_signed) rtl_sext(s, s1, s1, 1 << vtype->vsew);

  int idx;
  for(idx = vstart->val; idx < vl->val; idx ++) {
    rtlreg_t mask = get_mask(0, idx, vtype->vsew, vtype->vlmul);
    if(s->vm == 0) {
      // merge instr will exec no matter mask or not
      // masked and mask off exec will left dest unmodified.
      if(opcode != MERGE && mask==0) continue;
    } else if(opcode == MERGE) {
      mask = 1; // merge(mv) get the first operand (vs1, rs1, imm);
    }
    // operand - vs2
    get_vreg(id_src2->reg, idx, s0, vtype->vsew, vtype->vlmul, is_signed, 1);
    if(is_signed) rtl_sext(s, s0, s0, 1 << vtype->vsew);


    // op
    switch (opcode) {
      case REDSUM : rtl_add(s, s1, s0, s1); break;
      case REDOR  : rtl_or(s, s1, s0, s1); break;
      case REDAND : rtl_and(s, s1, s0, s1); break;
      case REDXOR : rtl_xor(s, s1, s0, s1); break;
      //  case MIN : 
      // MINU is hard to achieve parallel
    }

  }
  set_vreg(id_dest->reg, 0, vs1, vtype->vsew, vtype->vlmul, 0);
  
  int vlmax =  ((VLEN >> 3) >> vtype->vsew);
  for(int i=1; i<vlmax; i++) {
    set_vreg(id_dest->reg, i, 0, vtype->vsew, vtype->vlmul, 0);
  }
}

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
    if(vs0 == 1) break;
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
