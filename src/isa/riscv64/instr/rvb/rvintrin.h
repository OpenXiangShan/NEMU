/*
 *  RISC-V "B" extension proposal intrinsics and emulation
 *
 *  Copyright (C) 2019  Claire Wolf <claire@symbioticeda.com>
 *
 *  Permission to use, copy, modify, and/or distribute this software for any
 *  purpose with or without fee is hereby granted, provided that the above
 *  copyright notice and this permission notice appear in all copies.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 *  ----------------------------------------------------------------------
 *
 *  Define RVINTRIN_EMULATE to enable emulation mode.
 *
 *  This header defines C inline functions with "mockup intrinsics" for
 *  RISC-V "B" extension proposal instructions.
 *
 *  _rv_*(...)
 *    RV32/64 intrinsics that operate on the "long" data type
 *
 *  _rv32_*(...)
 *    RV32/64 intrinsics that operate on the "int32_t" data type
 *
 *  _rv64_*(...)
 *    RV64-only intrinsics that operate on the "int64_t" data type
 *
 */

#ifdef CONFIG_RVB

#include <limits.h>
#include <stdint.h>

int32_t _rv32_clz(int32_t rs1) { return rs1 ? __builtin_clz(rs1)   : 32; }
int64_t _rv64_clz(int64_t rs1) { return rs1 ? __builtin_clzll(rs1) : 64; }
int32_t _rv32_ctz(int32_t rs1) { return rs1 ? __builtin_ctz(rs1)   : 32; }
int64_t _rv64_ctz(int64_t rs1) { return rs1 ? __builtin_ctzll(rs1) : 64; }
int32_t _rv32_cpop(int32_t rs1) { return __builtin_popcount(rs1);   }
int64_t _rv64_cpop(int64_t rs1) { return __builtin_popcountll(rs1); }

int64_t _rv64_sext_b(int64_t rs1) { return rs1 << (64-8) >> (64-8); }
int64_t _rv64_sext_h(int64_t rs1) { return rs1 << (64-16) >> (64-16); }
int64_t _rv64_zext_h(int64_t rs1) { return (uint64_t)rs1 << (64-16) >> (64-16); }

int64_t _rv64_min (int64_t rs1, int64_t rs2) { return rs1 < rs2 ? rs1 : rs2; }
int64_t _rv64_minu(int64_t rs1, int64_t rs2) { return (uint64_t)rs1 < (uint64_t)rs2 ? rs1 : rs2; }
int64_t _rv64_max (int64_t rs1, int64_t rs2) { return rs1 > rs2 ? rs1 : rs2; }
int64_t _rv64_maxu(int64_t rs1, int64_t rs2) { return (uint64_t)rs1 > (uint64_t)rs2 ? rs1 : rs2; }

int64_t _rv64_bset (int64_t rs1, int64_t rs2) { return rs1 |  (1LL << (rs2 & 63)); }
int64_t _rv64_bclr (int64_t rs1, int64_t rs2) { return rs1 & ~(1LL << (rs2 & 63)); }
int64_t _rv64_binv (int64_t rs1, int64_t rs2) { return rs1 ^  (1LL << (rs2 & 63)); }
int64_t _rv64_bext (int64_t rs1, int64_t rs2) { return 1LL &  (rs1 >> (rs2 & 63)); }

int64_t _rv64_sll    (int64_t rs1, int64_t rs2) { return rs1 << (rs2 & 63); }
int64_t _rv64_srl    (int64_t rs1, int64_t rs2) { return (uint64_t)rs1 >> (rs2 & 63); }
int64_t _rv64_rol    (int64_t rs1, int64_t rs2) { return _rv64_sll(rs1, rs2) | _rv64_srl(rs1, -rs2); }
int64_t _rv64_ror    (int64_t rs1, int64_t rs2) { return _rv64_srl(rs1, rs2) | _rv64_sll(rs1, -rs2); }

int32_t _rv32_sll    (int32_t rs1, int32_t rs2) { return rs1 << (rs2 & 31); }
int32_t _rv32_srl    (int32_t rs1, int32_t rs2) { return (uint32_t)rs1 >> (rs2 & 31); }
int32_t _rv32_rol    (int32_t rs1, int32_t rs2) { return _rv32_sll(rs1, rs2) | _rv32_srl(rs1, -rs2); }
int32_t _rv32_ror    (int32_t rs1, int32_t rs2) { return _rv32_srl(rs1, rs2) | _rv32_sll(rs1, -rs2); }

int64_t _rv64_grev(int64_t rs1, int64_t rs2)
{
	uint64_t x = rs1;
	int shamt = rs2 & 63;
	if (shamt &  1) x = ((x & 0x5555555555555555LL) <<  1) | ((x & 0xAAAAAAAAAAAAAAAALL) >>  1);
	if (shamt &  2) x = ((x & 0x3333333333333333LL) <<  2) | ((x & 0xCCCCCCCCCCCCCCCCLL) >>  2);
	if (shamt &  4) x = ((x & 0x0F0F0F0F0F0F0F0FLL) <<  4) | ((x & 0xF0F0F0F0F0F0F0F0LL) >>  4);
	if (shamt &  8) x = ((x & 0x00FF00FF00FF00FFLL) <<  8) | ((x & 0xFF00FF00FF00FF00LL) >>  8);
	if (shamt & 16) x = ((x & 0x0000FFFF0000FFFFLL) << 16) | ((x & 0xFFFF0000FFFF0000LL) >> 16);
	if (shamt & 32) x = ((x & 0x00000000FFFFFFFFLL) << 32) | ((x & 0xFFFFFFFF00000000LL) >> 32);
	return x;
}

int64_t _rv64_gorc(int64_t rs1, int64_t rs2)
{
	uint64_t x = rs1;
	int shamt = rs2 & 63;
	if (shamt &  1) x |= ((x & 0x5555555555555555LL) <<  1) | ((x & 0xAAAAAAAAAAAAAAAALL) >>  1);
	if (shamt &  2) x |= ((x & 0x3333333333333333LL) <<  2) | ((x & 0xCCCCCCCCCCCCCCCCLL) >>  2);
	if (shamt &  4) x |= ((x & 0x0F0F0F0F0F0F0F0FLL) <<  4) | ((x & 0xF0F0F0F0F0F0F0F0LL) >>  4);
	if (shamt &  8) x |= ((x & 0x00FF00FF00FF00FFLL) <<  8) | ((x & 0xFF00FF00FF00FF00LL) >>  8);
	if (shamt & 16) x |= ((x & 0x0000FFFF0000FFFFLL) << 16) | ((x & 0xFFFF0000FFFF0000LL) >> 16);
	if (shamt & 32) x |= ((x & 0x00000000FFFFFFFFLL) << 32) | ((x & 0xFFFFFFFF00000000LL) >> 32);
	return x;
}


int64_t _rv64_clmul(int64_t rs1, int64_t rs2)
{
	uint64_t a = rs1, b = rs2, x = 0;
	for (int i = 0; i < 64; i++)
		if ((b >> i) & 1)
			x ^= a << i;
	return x;
}

int64_t _rv64_clmulh(int64_t rs1, int64_t rs2)
{
	uint64_t a = rs1, b = rs2, x = 0;
	for (int i = 1; i < 64; i++)
		if ((b >> i) & 1)
			x ^= a >> (64-i);
	return x;
}

int64_t _rv64_clmulr(int64_t rs1, int64_t rs2)
{
	uint64_t a = rs1, b = rs2, x = 0;
	for (int i = 0; i < 64; i++)
		if ((b >> i) & 1)
			x ^= a >> (63-i);
	return x;
}

long _rv_andn(long rs1, long rs2) { return rs1 & ~rs2; }
long _rv_orn (long rs1, long rs2) { return rs1 | ~rs2; }
long _rv_xnor(long rs1, long rs2) { return rs1 ^ ~rs2; }

long _rv_clz     (long rs1) { return _rv64_clz     (rs1); }
long _rv_ctz     (long rs1) { return _rv64_ctz     (rs1); }
long _rv_cpop    (long rs1) { return _rv64_cpop    (rs1); }
long _rv_sext_b  (long rs1) { return _rv64_sext_b  (rs1); }
long _rv_sext_h  (long rs1) { return _rv64_sext_h  (rs1); }
long _rv_zext_h  (long rs1) { return _rv64_zext_h  (rs1); }

long _rv_min      (long rs1, long rs2) { return _rv64_min      (rs1, rs2); }
long _rv_minu     (long rs1, long rs2) { return _rv64_minu     (rs1, rs2); }
long _rv_max      (long rs1, long rs2) { return _rv64_max      (rs1, rs2); }
long _rv_maxu     (long rs1, long rs2) { return _rv64_maxu     (rs1, rs2); }
long _rv_bset    (long rs1, long rs2) { return _rv64_bset    (rs1, rs2); }
long _rv_bclr    (long rs1, long rs2) { return _rv64_bclr    (rs1, rs2); }
long _rv_binv    (long rs1, long rs2) { return _rv64_binv    (rs1, rs2); }
long _rv_bext    (long rs1, long rs2) { return _rv64_bext    (rs1, rs2); }

long _rv_rol      (long rs1, long rs2) { return _rv64_rol      (rs1, rs2); }
long _rv_ror      (long rs1, long rs2) { return _rv64_ror      (rs1, rs2); }
long _rv_grev     (long rs1, long rs2) { return _rv64_grev     (rs1, rs2); }
long _rv_gorc     (long rs1, long rs2) { return _rv64_gorc     (rs1, rs2); }

long _rv_clmul    (long rs1, long rs2) { return _rv64_clmul    (rs1, rs2); }
long _rv_clmulh   (long rs1, long rs2) { return _rv64_clmulh   (rs1, rs2); }
long _rv_clmulr   (long rs1, long rs2) { return _rv64_clmulr   (rs1, rs2); }



#define RVINTRIN_GREV_PSEUDO_OP64(_arg, _name) \
	long    _rv_   ## _name(long    rs1) { return _rv_grev  (rs1, _arg); } 


RVINTRIN_GREV_PSEUDO_OP64(56, rev8)


#define RVINTRIN_GORC_PSEUDO_OP64(_arg, _name) \
	long    _rv_   ## _name(long    rs1) { return _rv_gorc  (rs1, _arg); } 

RVINTRIN_GORC_PSEUDO_OP64( 7, orc_b)


#endif // RVINTRIN_H
