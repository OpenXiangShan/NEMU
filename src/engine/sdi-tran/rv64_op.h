#ifndef __RV64_OP_H__
#define __RV64_OP_H__

#define rv64_add(c, a, b) do{ \
            uint8_t a_idx = reg_ptr2idx(s, a); \
            uint8_t b_idx = reg_ptr2idx(s, b); \
            uint8_t c_idx = reg_ptr2idx(s, c); \
            /*add c_idx,a_idx,b_idx*/ \
            printf("add x%d,x%d,x%d\n",c_idx,a_idx,b_idx); \
            }while(0)
#define rv64_sub(c, a, b) do{ \
            uint8_t a_idx = reg_ptr2idx(s, a); \
            uint8_t b_idx = reg_ptr2idx(s, b); \
            uint8_t c_idx = reg_ptr2idx(s, c); \
            /*sub c_idx,a_idx,b_idx*/ \
            printf("sub x%d,x%d,x%d\n",c_idx,a_idx,b_idx); \
            }while(0)
#define rv64_and(c, a, b) do{ \
            uint8_t a_idx = reg_ptr2idx(s, a); \
            uint8_t b_idx = reg_ptr2idx(s, b); \
            uint8_t c_idx = reg_ptr2idx(s, c); \
            /*and c_idx,a_idx,b_idx*/ \
            printf("and x%d,x%d,x%d\n",c_idx,a_idx,b_idx); \
            }while(0)
#define rv64_or(c, a, b) do{ \
            uint8_t a_idx = reg_ptr2idx(s, a); \
            uint8_t b_idx = reg_ptr2idx(s, b); \
            uint8_t c_idx = reg_ptr2idx(s, c); \
            /*or c_idx,a_idx,b_idx*/ \
            printf("or x%d,x%d,x%d\n",c_idx,a_idx,b_idx); \
            }while(0)
#define rv64_xor(c, a, b) do{ \
            uint8_t a_idx = reg_ptr2idx(s, a); \
            uint8_t b_idx = reg_ptr2idx(s, b); \
            uint8_t c_idx = reg_ptr2idx(s, c); \
            /*xor c_idx,a_idx,b_idx*/ \
            printf("xor x%d,x%d,x%d\n",c_idx,a_idx,b_idx); \
            }while(0)
#define rv64_shl(c, a, b) do{ \
            uint8_t a_idx = reg_ptr2idx(s, a); \
            uint8_t b_idx = reg_ptr2idx(s, b); \
            uint8_t c_idx = reg_ptr2idx(s, c); \
            /*sllw c_idx,a_idx,b_idx*/ \
            printf("sllw x%d,x%d,x%d\n",c_idx,a_idx,b_idx); \
            }while(0)
#define rv64_shr(c, a, b) do{ \
            uint8_t a_idx = reg_ptr2idx(s, a); \
            uint8_t b_idx = reg_ptr2idx(s, b); \
            uint8_t c_idx = reg_ptr2idx(s, c); \
            /*srlw c_idx,a_idx,b_idx*/ \
            printf("srlw x%d,x%d,x%d\n",c_idx,a_idx,b_idx); \
            }while(0)
#define rv64_sar(c, a, b) do{ \
            uint8_t a_idx = reg_ptr2idx(s, a); \
            uint8_t b_idx = reg_ptr2idx(s, b); \
            uint8_t c_idx = reg_ptr2idx(s, c); \
            /*sraw c_idx,a_idx,b_idx*/ \
            printf("sraw x%d,x%d,x%d\n",c_idx,a_idx,b_idx); \
            }while(0)

#define rv64_mul_lo(c, a, b) do{ \
            uint8_t a_idx = reg_ptr2idx(s, a); \
            uint8_t b_idx = reg_ptr2idx(s, b); \
            uint8_t c_idx = reg_ptr2idx(s, c); \
            /*mulw c_idx,a_idx,b_idx*/ \
            printf("mulw x%d,x%d,x%d\n",c_idx,a_idx,b_idx); \
            }while(0)
#define rv64_imul_lo(c, a, b) do{ \
            uint8_t a_idx = reg_ptr2idx(s, a); \
            uint8_t b_idx = reg_ptr2idx(s, b); \
            uint8_t c_idx = reg_ptr2idx(s, c); \
            /*mulw c_idx,a_idx,b_idx*/ \
            printf("mulw x%d,x%d,x%d\n",c_idx,a_idx,b_idx); \
            }while(0)
#ifdef ISA64
# define rv64_mul_hi(c, a, b) TODO()
# define rv64_imul_hi(c, a, b) TODO()
#else
# define rv64_mul_hi(c, a, b) do{ \
            uint8_t a_idx = reg_ptr2idx(s, a); \
            uint8_t b_idx = reg_ptr2idx(s, b); \
            uint8_t c_idx = reg_ptr2idx(s, c); \
            /*mulw c_idx,a_idx,b_idx*/ \
            printf("mulw x%d,x%d,x%d\n",c_idx,a_idx,b_idx); \
            }while(0)
# define rv64_imul_hi(c, a, b) do{ \
            uint8_t a_idx = reg_ptr2idx(s, a); \
            uint8_t b_idx = reg_ptr2idx(s, b); \
            uint8_t c_idx = reg_ptr2idx(s, c); \
            /*mul c_idx,a_idx,b_idx*/ \
            /*srai c_idx,c_idx,32*/ \
            printf("mul x%d,x%d,x%d\n",c_idx,a_idx,b_idx); \
            printf("srai x%d,x%d,0x20\n",c_idx,c_idx); \
            }while(0)
#endif

#define rv64_div_q(c, a, b) do{ \
            uint8_t a_idx = reg_ptr2idx(s, a); \
            uint8_t b_idx = reg_ptr2idx(s, b); \
            uint8_t c_idx = reg_ptr2idx(s, c); \
            /*divuw c_idx,a_idx,b_idx*/ \
            printf("divuw x%d,x%d,x%d\n",c_idx,a_idx,b_idx); \
            }while(0)
#define rv64_div_r(c, a, b)  do{ \
            uint8_t a_idx = reg_ptr2idx(s, a); \
            uint8_t b_idx = reg_ptr2idx(s, b); \
            uint8_t c_idx = reg_ptr2idx(s, c); \
            /*remuw c_idx,a_idx,b_idx*/ \
            printf("remuw x%d,x%d,x%d\n",c_idx,a_idx,b_idx); \
            }while(0)
#define rv64_idiv_q(c, a, b) do{ \
            uint8_t a_idx = reg_ptr2idx(s, a); \
            uint8_t b_idx = reg_ptr2idx(s, b); \
            uint8_t c_idx = reg_ptr2idx(s, c); \
            /*divw c_idx,a_idx,b_idx*/ \
            printf("divw x%d,x%d,x%d\n",c_idx,a_idx,b_idx); \
            }while(0)
#define rv64_idiv_r(c, a, b)  do{ \
            uint8_t a_idx = reg_ptr2idx(s, a); \
            uint8_t b_idx = reg_ptr2idx(s, b); \
            uint8_t c_idx = reg_ptr2idx(s, c); \
            /*remw c_idx,a_idx,b_idx*/ \
            printf("remw x%d,x%d,x%d\n",c_idx,a_idx,b_idx); \
            }while(0)

#endif
