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

#ifndef __RISCV64_RVK_CRYPTO_IMPL_H__
#define __RISCV64_RVK_CRYPTO_IMPL_H__

#ifdef CONFIG_RVK

#include <stdint.h>
#include <limits.h>

int32_t sha256sum0(int32_t rs1);
int32_t sha256sum1(int32_t rs1);
int32_t sha256sig0(int32_t rs1);
int32_t sha256sig1(int32_t rs1);

int64_t sha512sum0(int64_t rs1);
int64_t sha512sum1(int64_t rs1);
int64_t sha512sig0(int64_t rs1);
int64_t sha512sig1(int64_t rs1);

int32_t sm3p0(int32_t rs1);
int32_t sm3p1(int32_t rs1);

int64_t aes64es(int64_t rs1, int64_t rs2);
int64_t aes64esm(int64_t rs1, int64_t rs2);
int64_t aes64ds(int64_t rs1, int64_t rs2);
int64_t aes64dsm(int64_t rs1, int64_t rs2);
int64_t aes64im(int64_t rs1);
int64_t aes64ks1i(int64_t rs1, int64_t rs2);
int64_t aes64ks2(int64_t rs1, int64_t rs2);

int32_t sm4ed(int32_t rs1, int32_t rs2, uint8_t funct7);
int32_t sm4ks(int32_t rs1, int32_t rs2, uint8_t funct7);

#endif // CONFIG_RVK
#endif // __RISCV64_RVK_CRYPTO_IMPL_H__
