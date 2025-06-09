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

#ifndef __RISCV64_RVB_RVINTRIN_H__
#define __RISCV64_RVB_RVINTRIN_H__

#if defined(CONFIG_RVB) || defined(CONFIG_RVK)

#include <limits.h>
#include <stdint.h>

int32_t _rv32_clz(int32_t rs1);
int64_t _rv64_clz(int64_t rs1);
int32_t _rv32_ctz(int32_t rs1);
int64_t _rv64_ctz(int64_t rs1);
int32_t _rv32_cpop(int32_t rs1);
int64_t _rv64_cpop(int64_t rs1);

int64_t _rv64_sext_b(int64_t rs1);
int64_t _rv64_sext_h(int64_t rs1);
int64_t _rv64_zext_h(int64_t rs1);

int64_t _rv64_min(int64_t rs1, int64_t rs2);
int64_t _rv64_minu(int64_t rs1, int64_t rs2);
int64_t _rv64_max(int64_t rs1, int64_t rs2);
int64_t _rv64_maxu(int64_t rs1, int64_t rs2);

int64_t _rv64_bset(int64_t rs1, int64_t rs2);
int64_t _rv64_bclr(int64_t rs1, int64_t rs2);
int64_t _rv64_binv(int64_t rs1, int64_t rs2);
int64_t _rv64_bext(int64_t rs1, int64_t rs2);

int64_t _rv64_sll(int64_t rs1, int64_t rs2);
int64_t _rv64_srl(int64_t rs1, int64_t rs2);
int64_t _rv64_rol(int64_t rs1, int64_t rs2);
int64_t _rv64_ror(int64_t rs1, int64_t rs2);

int32_t _rv32_sll(int32_t rs1, int32_t rs2);
int32_t _rv32_srl(int32_t rs1, int32_t rs2);
int32_t _rv32_rol(int32_t rs1, int32_t rs2);
int32_t _rv32_ror(int32_t rs1, int32_t rs2);

int32_t _rv32_pack(int32_t rs1, int32_t rs2);
int64_t _rv64_pack(int64_t rs1, int64_t rs2);
int64_t _rv64_packh(int64_t rs1, int64_t rs2);

int64_t _rv64_grev(int64_t rs1, int64_t rs2);

int64_t _rv64_gorc(int64_t rs1, int64_t rs2);

int64_t _rv64_clmul(int64_t rs1, int64_t rs2);
int64_t _rv64_clmulh(int64_t rs1, int64_t rs2);
int64_t _rv64_clmulr(int64_t rs1, int64_t rs2);

int64_t _rv64_xpermn(int64_t rs1, int64_t rs2);
int64_t _rv64_xpermb(int64_t rs1, int64_t rs2);
// int64_t _rv64_xpermh(int64_t rs1, int64_t rs2);
// int64_t _rv64_xpermw(int64_t rs1, int64_t rs2);

long _rv_andn(long rs1, long rs2);
long _rv_orn(long rs1, long rs2);
long _rv_xnor(long rs1, long rs2);

long _rv_clz(long rs1);
long _rv_ctz(long rs1);
long _rv_cpop(long rs1);
long _rv_sext_b(long rs1);
long _rv_sext_h(long rs1);
long _rv_zext_h(long rs1);

long _rv_min(long rs1, long rs2);
long _rv_minu(long rs1, long rs2);
long _rv_max(long rs1, long rs2);
long _rv_maxu(long rs1, long rs2);

long _rv_bset(long rs1, long rs2);
long _rv_bclr(long rs1, long rs2);
long _rv_binv(long rs1, long rs2);
long _rv_bext(long rs1, long rs2);

long _rv_rol(long rs1, long rs2);
long _rv_ror(long rs1, long rs2);
long _rv_grev(long rs1, long rs2);
long _rv_gorc(long rs1, long rs2);

long _rv_clmul(long rs1, long rs2);
long _rv_clmulh(long rs1, long rs2);
long _rv_clmulr(long rs1, long rs2);

long _rv_pack(long rs1, long rs2);
long _rv_packh(long rs1, long rs2);
long _rv_xpermn(long rs1, long rs2);
long _rv_xpermb(long rs1, long rs2);


#define RVINTRIN_GREV_PSEUDO_OP64(_arg, _name) \
	long    _rv_   ## _name(long    rs1);


RVINTRIN_GREV_PSEUDO_OP64(56, rev8)
RVINTRIN_GREV_PSEUDO_OP64(7, revb)

#undef RVINTRIN_GREV_PSEUDO_OP64


#define RVINTRIN_GORC_PSEUDO_OP64(_arg, _name) \
	long    _rv_   ## _name(long    rs1);

RVINTRIN_GORC_PSEUDO_OP64( 7, orc_b)

#undef RVINTRIN_GORC_PSEUDO_OP64


#endif // defined(CONFIG_RVB) || defined(CONFIG_RVK)
#endif // __RISCV64_RVB_RVINTRIN_H__
