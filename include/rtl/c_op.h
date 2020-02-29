#ifndef __RTL_C_OP_H__
#define __RTL_C_OP_H__

#define c_add(a, b) ((a) + (b))
#define c_sub(a, b) ((a) - (b))
#define c_and(a, b) ((a) & (b))
#define c_or(a, b)  ((a) | (b))
#define c_xor(a, b) ((a) ^ (b))
#define c_shl(a, b) ((a) << (b))
#define c_shr(a, b) ((a) >> (b))
#define c_sar(a, b) ((sword_t)(a) >> (b))

#define c_mul_lo(a, b) ((a) * (b))
#define c_imul_lo(a, b) ((sword_t)(a) * (sword_t)(b))
#ifdef ISA64
# define c_mul_hi(a, b) (((__uint128_t)(a) * (__uint128_t)(b)) >> 64)
# define c_imul_hi(a, b) (((__int128_t)(sword_t)(a) * (__int128_t)(sword_t)(b)) >> 64)
#else
# define c_mul_hi(a, b) (((uint64_t)(a) * (uint64_t)(b)) >> 32)
# define c_imul_hi(a, b) (((int64_t)(sword_t)(a) * (int64_t)(sword_t)(b)) >> 32)
#endif

#define c_div_q(a, b) ((a) / (b))
#define c_div_r(a, b)  ((a) % (b))
#define c_idiv_q(a, b) ((sword_t)(a) / (sword_t)(b))
#define c_idiv_r(a, b)  ((sword_t)(a) % (sword_t)(b))

#endif
