#include <rtl/rtl.h>
#include "rv64_op.h"
#include "rv_ins_def.h"

void rv64_relop(uint32_t relop, const rtlreg_t* dest, const rtlreg_t* src1, const rtlreg_t* src2);
uint8_t reg_ptr2idx(DecodeExecState *s, const rtlreg_t* dest);

/* RTL basic instructions */

make_rtl(li, rtlreg_t* dest, rtlreg_t imm) {
  uint8_t idx=reg_ptr2idx(s, dest);
  RV_IMM rv_imm;
  rv_imm.val = imm;
  //lui idx,rv_imm.imm_hi20
  //lui x31,rv_imm.imm_lo12
  //srli x31,x31,12
  //or idx,idx,x31
  printf("lui x%d,0x%x\n",idx,rv_imm.imm_11_0);
  printf("lui x31,0x%x\n",rv_imm.imm_31_12);
  printf ("srli x31,x31,12\n");
  printf("or x%d,x%d,x31\n",idx,idx);
}


make_rtl(mv, rtlreg_t* dest, const rtlreg_t *src1) {
  uint8_t idx_dest,idx_src1;
  idx_dest=reg_ptr2idx(s, dest);
  idx_src1=reg_ptr2idx(s, src1);
  //addiw idx_dest,idx_src1,0
  printf("addiw x%d,x%d,0\n",idx_dest,idx_src1);
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
  printf("slli x31,x%d,32\n",idx_addr);
  printf("srli x31,x31,32\n");
  switch (len)
  {
  case 1:
    //lb dest,addr
    printf("lb x%d,x31\n",idx_dest);
    break;
  case 2:
    //lh dest,addr
    printf("lh x%d,x31\n",idx_dest);
    break;
  default://4
    //lw dest,addr
    printf("lw x%d,x31\n",idx_dest);
    break;
  }
}

make_rtl(sm, const rtlreg_t* addr, const rtlreg_t* src1, int len) {
  uint8_t idx_addr,idx_src1;
  idx_addr=reg_ptr2idx(s, addr);
  idx_src1=reg_ptr2idx(s, src1);
  printf("slli x31,x%d,32\n",idx_addr);
  printf("srli x31,x31,32\n");
  switch (len)
  {
  case 1:
    //sb addr,src1
    printf("sb x31,x%d\n",idx_src1);
    break;
  case 2:
    //sh addr,src1
    printf("sh x31,x%d\n",idx_src1);
    break;
  default://4
    //sw addr,src1
    printf("sw x31,x%d\n",idx_src1);
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
        //slliw dest,r,16
        //sraiw dest,dest,24
        printf("slliw x%d,x%d,16\n",idx_dest,idx_r);
        printf("sraiw x%d,x%d,16\n",idx_dest,idx_dest);
      }else{//low
        //slliw dest,r,24
        //sraiw dest,dest,24
        printf("slliw x%d,x%d,24\n",idx_dest,idx_r);
        printf("sraiw x%d,x%d,24\n",idx_dest,idx_dest);
      }
      return;
    case 2:
      //slliw dest,r,16
      //sraiw dest,dest,16
      printf("slliw x%d,x%d,16\n",idx_dest,idx_r);
      printf("sraiw x%d,x%d,16\n",idx_dest,idx_dest);
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
        //and r,r,x31
        //slliw x30,src1,8
        //xor x31,x31,x0
        //and x30,x30,x31
        //or r,r,x30
        printf("addi x31,x0,0xff\n");
        printf("slliw x31,x31,8\n");
        printf("and x%d,x%d,x31\n",idx_r,idx_r);
        printf("slliw x30,x%d,8\n",idx_src1);
        printf("xor x31,x31,x0\n");
        printf("and x30,x30,x31\n");
        printf("or x%d,x%d,x30\n",idx_r,idx_r);
      }else{//low
        //addi x31,x0,0xff
        //xor x30,x0,x31
        //and x31,x31,r
        //andi x30,x30,dest
        //or dest,x30,x31
        printf("addi x31,0xff\n");
        printf("xor x30,x0,x31\n");
        printf("and x31,x31,x%d\n",idx_r);
        printf("and x30,x30,x%d\n",idx_src1);
        printf("or x%d,x30,x31\n",idx_r);
      }
      return;
    case 2:
      //lui x31,0xffff0
      //xor x30,x31,x0
      //and x31,x31,r
      //and x30,x30,src1
      //or r,x30,x31
      printf("lui x31,0xffff0\n");
      printf("xor x30,x0,x31\n");
      printf("and x31,x31,x%d\n",idx_r);
      printf("and x30,x30,x%d\n",idx_src1);
      printf("or x%d,x30,x31\n",idx_r);
      return;
    default: assert(0);
  }
}

//relop
make_rtl(setrelop, uint32_t relop, rtlreg_t *dest, const rtlreg_t *src1, const rtlreg_t *src2) {
  rv64_relop(relop, dest, src1, src2);
}

//jump
make_rtl(j, vaddr_t target) {
  TODO();
}

make_rtl(jr, rtlreg_t *target) {
  TODO();
}

make_rtl(jrelop, uint32_t relop, const rtlreg_t *src1, const rtlreg_t *src2, vaddr_t target) {
  TODO();
}
