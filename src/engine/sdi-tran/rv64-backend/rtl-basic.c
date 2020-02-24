#include <rtl/rtl.h>
#include "rv64_op.h"
#include "rv_ins_def.h"
#include "../sdi.h"

void rv64_relop(DecodeExecState *s, uint32_t relop,
    const rtlreg_t* dest, const rtlreg_t* src1, const rtlreg_t* src2);
uint8_t reg_ptr2idx(DecodeExecState *s, const rtlreg_t* dest);
extern int tran_is_jmp;

/* RTL basic instructions */

make_rtl(li, rtlreg_t* dest, rtlreg_t imm) {
  uint8_t idx=reg_ptr2idx(s, dest);
  RV_IMM rv_imm;
  rv_imm.val = imm;
  //lui idx,rv_imm.imm_hi20
  //lui x31,rv_imm.imm_lo12
  //srliw x31,x31,12
  //or idx,idx,x31
  //printf("lui x%d,0x%x\n",idx,rv_imm.imm_31_12);
  //printf("lui x31,0x%x\n",rv_imm.imm_11_0);
  //printf ("srliw x31,x31,12\n");
  //printf("or x%d,x%d,x31\n",idx,idx);
  gen_rv64_U_inst(LUI_OP,idx,rv_imm.imm_31_12);
  gen_rv64_U_inst(LUI_OP,31,rv_imm.imm_11_0);
  gen_rv64_R_inst(SRLIW_OP,31,SRLIW_FUNCT3,31,12,SRLIW_FUNCT7);
  gen_rv64_R_inst(OR_OP,idx,OR_FUNCT3,idx,31,OR_FUNCT7);
}


make_rtl(mv, rtlreg_t* dest, const rtlreg_t *src1) {
  uint8_t idx_dest,idx_src1;
  idx_dest=reg_ptr2idx(s, dest);
  idx_src1=reg_ptr2idx(s, src1);
  //addiw idx_dest,idx_src1,0
  //printf("addiw x%d,x%d,0\n",idx_dest,idx_src1);
  gen_rv64_I_inst(ADDIW_OP,idx_dest,ADDIW_FUNCT3,idx_src1,0);
}

#define make_rtl_arith_logic(name) \
  make_rtl(name, rtlreg_t* dest, const rtlreg_t* src1, const rtlreg_t* src2) { \
    concat(rv64_, name) (dest, src1, src2); \
  }

make_rtl_arith_logic(add)
make_rtl_arith_logic(sub)
make_rtl_arith_logic(and)
make_rtl_arith_logic(or)
make_rtl_arith_logic(xor)
make_rtl_arith_logic(shl)
make_rtl_arith_logic(shr)
make_rtl_arith_logic(sar)
make_rtl_arith_logic(mul_lo)
make_rtl_arith_logic(mul_hi)
make_rtl_arith_logic(imul_lo)
make_rtl_arith_logic(imul_hi)
make_rtl_arith_logic(div_q)
make_rtl_arith_logic(div_r)
make_rtl_arith_logic(idiv_q)
make_rtl_arith_logic(idiv_r)

make_rtl(div64_q, rtlreg_t* dest,
    const rtlreg_t* src1_hi, const rtlreg_t* src1_lo, const rtlreg_t* src2) {
  TODO();
}

make_rtl(div64_r, rtlreg_t* dest,
    const rtlreg_t* src1_hi, const rtlreg_t* src1_lo, const rtlreg_t* src2) {
  TODO();
}

make_rtl(idiv64_q, rtlreg_t* dest,
    const rtlreg_t* src1_hi, const rtlreg_t* src1_lo, const rtlreg_t* src2) {
  TODO();
}

make_rtl(idiv64_r, rtlreg_t* dest,
    const rtlreg_t* src1_hi, const rtlreg_t* src1_lo, const rtlreg_t* src2) {
  TODO();
}
//memory access
//lm sm need to suppose addr is unsigned
make_rtl(lm, rtlreg_t *dest, const rtlreg_t* addr, int len) {
  uint8_t idx_dest,idx_addr;
  idx_dest=reg_ptr2idx(s, dest);
  idx_addr=reg_ptr2idx(s, addr);
  //printf("slli x31,x%d,32\n",idx_addr);
  //printf("srli x31,x31,32\n");
  gen_rv64_R64_inst(SLLI_OP,31,SLLI_FUNCT3,idx_addr,32,SLLI_FUNCT6);
  gen_rv64_R64_inst(SRLI_OP,31,SRLI_FUNCT3,31,32,SRLI_FUNCT6);
  switch (len)
  {
  case 1:
    //lb dest,addr
    //printf("lb x%d,0(x31)\n",idx_dest);
    gen_rv64_I_inst(LB_OP,idx_dest,LB_FUNCT3,31,0);
    break;
  case 2:
    //lh dest,addr
    //printf("lh x%d,0(x31)\n",idx_dest);
    gen_rv64_I_inst(LH_OP,idx_dest,LH_FUNCT3,31,0);
    break;
  default://4
    //lw dest,addr
    //printf("lw x%d,0(x31)\n",idx_dest);
    gen_rv64_I_inst(LW_OP,idx_dest,LW_FUNCT3,31,0);
    break;
  }
}

make_rtl(sm, const rtlreg_t* addr, const rtlreg_t* src1, int len) {
  uint8_t idx_addr,idx_src1;
  idx_addr=reg_ptr2idx(s, addr);
  idx_src1=reg_ptr2idx(s, src1);
  //printf("slli x31,x%d,32\n",idx_addr);
  //printf("srli x31,x31,32\n");
  gen_rv64_R64_inst(SLLI_OP,31,SLLI_FUNCT3,idx_addr,32,SLLI_FUNCT6);
  gen_rv64_R64_inst(SRLI_OP,31,SRLI_FUNCT3,31,32,SRLI_FUNCT6);
  switch (len)
  {
  case 1:
    //sb addr,src1
    //printf("sb x%d,0(x31)\n",idx_src1);
    gen_rv64_S_inst(SB_OP,SB_FUNCT3,31,idx_src1,0);
    break;
  case 2:
    //sh addr,src1
    //printf("sh x%d,0(x31)\n",idx_src1);
    gen_rv64_S_inst(SH_OP,SH_FUNCT3,31,idx_src1,0);
    break;
  default://4
    //sw addr,src1
    //printf("sw x%d,0(x31)\n",idx_src1);
    gen_rv64_S_inst(SW_OP,SW_FUNCT3,31,idx_src1,0);
    break;
  }
}

//host memory access
//we can ignore this impl
make_rtl(host_lm, rtlreg_t* dest, const void *addr, int len) {
  uint8_t idx_dest=reg_ptr2idx(s, dest);

  // we assume that `addr` is only from cpu.gpr in x86
  uintptr_t addr_align = (uintptr_t)addr & ~(sizeof(rtlreg_t) - 1);
  uint8_t idx_r = reg_ptr2idx(s, (void *)addr_align);
  switch (len) {
    case 1:
      if((uintptr_t)addr & 1){//high
        //slli dest,r,48
        //srli dest,dest,56
        //printf("slli x%d,x%d,48\n",idx_dest,idx_r);
        //printf("srli x%d,x%d,56\n",idx_dest,idx_dest);
        gen_rv64_R64_inst(SLLI_OP,idx_dest,SLLI_FUNCT3,idx_r,48,SLLI_FUNCT6);
        gen_rv64_R64_inst(SRLI_OP,idx_dest,SRLI_FUNCT3,idx_dest,56,SRLI_FUNCT6);
      }else{//low
        //slli dest,r,56
        //srli dest,dest,56
        //printf("slli x%d,x%d,56\n",idx_dest,idx_r);
        //printf("srli x%d,x%d,56\n",idx_dest,idx_dest);
        gen_rv64_R64_inst(SLLI_OP,idx_dest,SLLI_FUNCT3,idx_r,56,SLLI_FUNCT6);
        gen_rv64_R64_inst(SRLI_OP,idx_dest,SRLI_FUNCT3,idx_dest,56,SRLI_FUNCT6);
      }
      return;
    case 2:
      //slli dest,r,48
      //srli dest,dest,48
      //printf("slli x%d,x%d,48\n",idx_dest,idx_r);
      //printf("srli x%d,x%d,48\n",idx_dest,idx_dest);
      gen_rv64_R64_inst(SLLI_OP,idx_dest,SLLI_FUNCT3,idx_r,48,SLLI_FUNCT6);
      gen_rv64_R64_inst(SRLI_OP,idx_dest,SRLI_FUNCT3,idx_dest,48,SRLI_FUNCT6);
      return;
    default: assert(0);
  }
}

make_rtl(host_sm, void *addr, const rtlreg_t *src1, int len) {
  uint8_t idx_src1=reg_ptr2idx(s, src1);

  // we assume that `addr` is only from cpu.gpr in x86
  uintptr_t addr_align = (uintptr_t)addr & ~(sizeof(rtlreg_t) - 1);
  uint8_t idx_r = reg_ptr2idx(s, (void *)addr_align);
  switch (len) {
    case 1:
      if((uintptr_t)addr & 1){//high
        //addi x31,x0,0xff
        //slliw x31,x31,8
        //slliw x30,src1,8
        //and x30,x30,x31
        //xor x31,x31,x0
        //and r,r,x31
        //or r,r,x30
        //printf("addi x31,x0,0xff\n");
        //printf("slliw x31,x31,8\n");
        //printf("slliw x30,x%d,8\n",idx_src1);
        //printf("and x30,x30,x31\n");
        //printf("xor x31,x31,x0\n");
        //printf("and x%d,x%d,x31\n",idx_r,idx_r);
        //printf("or x%d,x%d,x30\n",idx_r,idx_r);
        gen_rv64_I_inst(ADDI_OP,31,ADDI_FUNCT3,0,0xff);
        gen_rv64_R_inst(SLLIW_OP,31,SLLIW_FUNCT3,31,8,SLLIW_FUNCT7);
        gen_rv64_R_inst(SLLIW_OP,30,SLLIW_FUNCT3,idx_src1,8,SLLIW_FUNCT7);
        gen_rv64_R_inst(AND_OP,30,AND_FUNCT3,30,31,AND_FUNCT7);
        gen_rv64_R_inst(XOR_OP,31,XOR_FUNCT3,31,0,XOR_FUNCT7);
        gen_rv64_R_inst(AND_OP,idx_r,AND_FUNCT3,idx_r,31,AND_FUNCT7);
        gen_rv64_R_inst(OR_OP,idx_r,OR_FUNCT3,idx_r,30,OR_FUNCT7);
      }else{//low
        //addi x31,x0,0xff
        //xor x30,x0,x31
        //and x31,x31,src1
        //andi x30,x30,r
        //or dest,x30,x31
        //printf("addi x31,x0,0xff\n");
        //printf("xor x30,x0,x31\n");
        //printf("and x31,x31,x%d\n",idx_src1);
        //printf("and x30,x30,x%d\n",idx_r);
        //printf("or x%d,x30,x31\n",idx_r);
        gen_rv64_I_inst(ADDI_OP,31,ADDI_FUNCT3,0,0xff);
        gen_rv64_R_inst(XOR_OP,30,XOR_FUNCT3,0,31,XOR_FUNCT7);
        gen_rv64_R_inst(AND_OP,31,AND_FUNCT3,31,idx_src1,AND_FUNCT7);
        gen_rv64_R_inst(AND_OP,30,AND_FUNCT3,30,idx_r,AND_FUNCT7);
        gen_rv64_R_inst(OR_OP,idx_r,OR_FUNCT3,30,31,OR_FUNCT7);
      }
      return;
    case 2:
      //lui x31,0xffff0
      //xor x30,x31,x0
      //and x31,x31,r
      //and x30,x30,src1
      //or r,x30,x31
      //printf("lui x31,0xffff0\n");
      //printf("xor x30,x0,x31\n");
      //printf("and x31,x31,x%d\n",idx_r);
      //printf("and x30,x30,x%d\n",idx_src1);
      //printf("or x%d,x30,x31\n",idx_r);
      gen_rv64_U_inst(LUI_OP,31,0xffff0);
      gen_rv64_R_inst(XOR_OP,30,XOR_FUNCT3,0,31,XOR_FUNCT7);
      gen_rv64_R_inst(AND_OP,31,AND_FUNCT3,31,idx_r,AND_FUNCT7);
      gen_rv64_R_inst(AND_OP,30,AND_FUNCT3,30,idx_src1,AND_FUNCT7);
      gen_rv64_R_inst(OR_OP,idx_r,OR_FUNCT3,30,31,OR_FUNCT7);
      return;
    default: assert(0);
  }
}

//relop
make_rtl(setrelop, uint32_t relop, rtlreg_t *dest, const rtlreg_t *src1, const rtlreg_t *src2) {
  rv64_relop(s, relop, dest, src1, src2);
}

//jump
make_rtl(j, vaddr_t target) {
  // cpu.pc = target;
  rtl_li(s, t0, target);
  gen_rv64_U_inst(LUI_OP,5,JMP_MMIO_BASE>>12);//lui t1,JMP_MMIO_BASE>>12
  gen_rv64_S_inst(SW_OP,SW_FUNCT3,5,4,JMP_SPC);//sw t0, SPC(t1), store target spc
  gen_rv64_I_inst(ADDI_OP,31,ADDI_FUNCT3,0,1);
  gen_rv64_S_inst(SW_OP,SW_FUNCT3,5,31,JMP_VALID);//sw x31, VALID(t1), set valid
  gen_rv64_I_inst(LW_OP,31,LW_FUNCT3,5,JMP_VALID);//lw x31, VALID(t1), read valid
  gen_rv64_B_inst(BNE_OP,BNE_FUNCT3,0,31,-4);//bne x0,x31,-4 wait until clear
  gen_rv64_I_inst(LWU_OP,31,LWU_FUNCT3,5,JMP_TARGET);//lwu x31, TARGET(t1), load target
  gen_rv64_I_inst(JALR_OP,0,JALR_FUNCT3,31,0);//jalr x0,x31,0
  s->is_jmp = true;
  tran_is_jmp = true;
}

make_rtl(jr, rtlreg_t *target) {
  // cpu.pc = target;
  rtl_mv(s, t0, target);
  gen_rv64_U_inst(LUI_OP,5,JMP_MMIO_BASE>>12);//lui t1,JMP_MMIO_BASE>>12
  gen_rv64_S_inst(SW_OP,SW_FUNCT3,5,4,JMP_SPC);//sw t0, SPC(t1), store target spc
  gen_rv64_I_inst(ADDI_OP,31,ADDI_FUNCT3,0,1);
  gen_rv64_S_inst(SW_OP,SW_FUNCT3,5,31,JMP_VALID);//sw x31, VALID(t1), set valid
  gen_rv64_I_inst(LWU_OP,31,LWU_FUNCT3,5,JMP_VALID);//lwu x31, VALID(t1), read valid
  gen_rv64_B_inst(BNE_OP,BNE_FUNCT3,0,31,-4);//bne x0,x31,-4 wait until clear
  gen_rv64_I_inst(LWU_OP,31,LWU_FUNCT3,5,JMP_TARGET);//lwu x31, TARGET(t1), load target
  gen_rv64_I_inst(JALR_OP,0,JALR_FUNCT3,31,0);//jalr x0,x31,0
  s->is_jmp = true;
  tran_is_jmp = true;
}

make_rtl(jrelop, uint32_t relop, const rtlreg_t *src1, const rtlreg_t *src2, vaddr_t target) {
  rtl_li(s, t0, s->seq_pc);
  rtl_li(s, t1, target);
  //assume it will always be src1 != 0
  //and src1 will only be 0/1
  assert(relop == RELOP_NE);

  //using mux
  gen_rv64_I_inst(ADDIW_OP,31,ADDIW_FUNCT3,reg_ptr2idx(s, src1),-1);
  // x31 = mask
  gen_rv64_R_inst(AND_OP,30,AND_FUNCT3,4,31,AND_FUNCT7);//and x30,t0,x31
  gen_rv64_I_inst(XORI_OP,31,XOR_FUNCT3,31,-1);//xori x31,x31,-1
  gen_rv64_R_inst(AND_OP,31,AND_FUNCT3,5,31,AND_FUNCT7);//and x31,t1,x31
  gen_rv64_R_inst(OR_OP,4,OR_FUNCT3,30,31,OR_FUNCT7);//or t0,x30,x31

  /*using branch
  // gen_rv64_B_inst(BNE_OP,BNE_FUNCT3,reg_ptr2idx(s, src1),0,24);//5 insts in total, because rtl_li is consist of 4 insts and a jmp inst
  // rtl_li(&t0,decinfo.seq_pc);
  // gen_rv64_J_inst(JAL_OP,0,20);//rtl_li is consist of 4 insts
  // rtl_li(&t0,target);*/

  gen_rv64_U_inst(LUI_OP,5,JMP_MMIO_BASE>>12);//lui t1,JMP_MMIO_BASE>>12
  gen_rv64_S_inst(SW_OP,SW_FUNCT3,5,4,JMP_SPC);//sw t0, SPC(t1), store target spc
  gen_rv64_I_inst(ADDI_OP,31,ADDI_FUNCT3,0,1);
  gen_rv64_S_inst(SW_OP,SW_FUNCT3,5,31,JMP_VALID);//sw x31, VALID(t1), set valid
  gen_rv64_I_inst(LWU_OP,31,LWU_FUNCT3,5,JMP_VALID);//lwu x31, VALID(t1), read valid
  gen_rv64_B_inst(BNE_OP,BNE_FUNCT3,0,31,-4);//bne x0,x31,-4 wait until clear
  gen_rv64_I_inst(LWU_OP,31,LWU_FUNCT3,5,JMP_TARGET);//lwu x31, TARGET(t1), load target
  gen_rv64_I_inst(JALR_OP,0,JALR_FUNCT3,31,0);//jalr x0,x31,0

  s->is_jmp = true;
  tran_is_jmp = true;
}
