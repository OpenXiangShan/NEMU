#include <common.h>
#include <rtl/rtl.h>

uint8_t reg_ptr2idx(DecodeExecState *s, const rtlreg_t* dest);

void rv64_relop(DecodeExecState *s, uint32_t relop, 
    const rtlreg_t* dest, const rtlreg_t* src1, const rtlreg_t* src2) {
  uint8_t idx_dest,idx_src1,idx_src2;
  idx_dest=reg_ptr2idx(s, dest);
  idx_src1=reg_ptr2idx(s, src1);
  idx_src2=reg_ptr2idx(s, src2);
  switch (relop) {
    case RELOP_FALSE: //xor dest,x0,x0
      printf("add x%x,x0,x0\n",idx_dest);return;
    case RELOP_TRUE: //xor dest,x0,x0
      printf("addi x%x,x0,1\n",idx_dest);return;
    case RELOP_EQ:
      //xor x31,src1,src2
      //sltu dest,x0,x31
      //xori dest,dest,1
      printf("xor x31,x%d,x%d\n",idx_src1,idx_src2);
      printf("sltu x%d,x0,x31\n",idx_dest);
      printf("xori x%d,x%d,1\n",idx_dest,idx_dest);
      return ;
    case RELOP_NE:
      //xor x31,src1,src2
      //sltu dest,x0,x31
      printf("xor x31,x%d,x%d\n",idx_src1,idx_src2);
      printf("sltu x%d,x0,x31\n",idx_dest);
      return ;
    case RELOP_LT:
      //slt dest,src1,src2
      printf("slt x%d,x%d,x%d\n",idx_dest,idx_src1,idx_src2);
      return ;
    case RELOP_LE:
      //slt dest,src2,src1
      //xori dest,dest,1
      printf("slt x%d,x%d,x%d\n",idx_dest,idx_src2,idx_src1);
      printf("xori x%d,x%d,1\n",idx_dest,idx_dest);
      return ;
    case RELOP_GT:
      //slt dest,src2,src1
      printf("slt x%d,x%d,x%d\n",idx_dest,idx_src2,idx_src1);
      return ;
    case RELOP_GE:
      //slt dest,src1,src2
      //xori dest,dest,1
      printf("slt x%d,x%d,x%d\n",idx_dest,idx_src1,idx_src2);
      printf("xori x%d,x%d,1\n",idx_dest,idx_dest);
      return ;
    case RELOP_LTU:
      //sltu dest,src1,src2
      printf("sltu x%d,x%d,x%d\n",idx_dest,idx_src1,idx_src2);
      return ;
    case RELOP_LEU:
      //sltu dest,src2,src1
      //xori dest,dest,1
      printf("sltu x%d,x%d,x%d\n",idx_dest,idx_src2,idx_src1);
      printf("xori x%d,x%d,1\n",idx_dest,idx_dest);
      return ;
    case RELOP_GTU:
      //sltu dest,src2,src1
      printf("sltu x%d,x%d,x%d\n",idx_dest,idx_src2,idx_src1);
      return ;
    case RELOP_GEU:
      //sltu dest,src1,src2
      //xori dest,dest,1
      printf("sltu x%d,x%d,x%d\n",idx_dest,idx_src1,idx_src2);
      printf("xori x%d,x%d,1\n",idx_dest,idx_dest);
      return ;
    default: panic("unsupport relop = %d", relop);
  }
}
