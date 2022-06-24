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

#ifdef CONFIG_RVK

#include <stdint.h>
#include <limits.h>
#include "aes_common.h"
#include "sm4_common.h"


int32_t sha256sum0 (int32_t rs1) { return _rv32_ror(rs1, 2)  ^ _rv32_ror(rs1, 13) ^ _rv32_ror(rs1, 22); }
int32_t sha256sum1 (int32_t rs1) { return _rv32_ror(rs1, 6)  ^ _rv32_ror(rs1, 11) ^ _rv32_ror(rs1, 25); }
int32_t sha256sig0 (int32_t rs1) { return _rv32_ror(rs1, 7)  ^ _rv32_ror(rs1, 18) ^ ((uint32_t)rs1 >> 3); }
int32_t sha256sig1 (int32_t rs1) { return _rv32_ror(rs1, 17) ^ _rv32_ror(rs1, 19) ^ ((uint32_t)rs1 >> 10); }

int64_t sha512sum0 (int64_t rs1) { return _rv64_ror(rs1, 28) ^ _rv64_ror(rs1, 34) ^ _rv64_ror(rs1, 39); }
int64_t sha512sum1 (int64_t rs1) { return _rv64_ror(rs1, 14) ^ _rv64_ror(rs1, 18) ^ _rv64_ror(rs1, 41); }
int64_t sha512sig0 (int64_t rs1) { return _rv64_ror(rs1, 1)  ^ _rv64_ror(rs1, 8)  ^ ((uint64_t)rs1 >> 7); }
int64_t sha512sig1 (int64_t rs1) { return _rv64_ror(rs1, 19) ^ _rv64_ror(rs1, 61) ^ ((uint64_t)rs1 >> 6); }

int32_t sm3p0 (int32_t rs1) { return rs1 ^ _rv32_rol(rs1, 9)  ^ _rv32_rol(rs1, 17); }
int32_t sm3p1 (int32_t rs1) { return rs1 ^ _rv32_rol(rs1, 15) ^ _rv32_rol(rs1, 23); }

int64_t aes64es (int64_t rs1, int64_t rs2)
{
    uint64_t temp = AES_SHIFROWS_LO(rs1,rs2);
    return  ((uint64_t)AES_ENC_SBOX[(temp >>  0) & 0xFF] <<  0) |
            ((uint64_t)AES_ENC_SBOX[(temp >>  8) & 0xFF] <<  8) |
            ((uint64_t)AES_ENC_SBOX[(temp >> 16) & 0xFF] << 16) |
            ((uint64_t)AES_ENC_SBOX[(temp >> 24) & 0xFF] << 24) |
            ((uint64_t)AES_ENC_SBOX[(temp >> 32) & 0xFF] << 32) |
            ((uint64_t)AES_ENC_SBOX[(temp >> 40) & 0xFF] << 40) |
            ((uint64_t)AES_ENC_SBOX[(temp >> 48) & 0xFF] << 48) |
            ((uint64_t)AES_ENC_SBOX[(temp >> 56) & 0xFF] << 56) ;
}

int64_t aes64esm (int64_t rs1, int64_t rs2)
{ 
    uint64_t temp = aes64es(rs1, rs2);
    uint32_t col_0 = temp & 0xFFFFFFFF;
    uint32_t col_1 = temp >> 32;
    col_0 = AES_MIXCOLUMN(col_0);
    col_1 = AES_MIXCOLUMN(col_1);
    return ((uint64_t)col_1 << 32) | col_0;
}

int64_t aes64ds (int64_t rs1, int64_t rs2)
{
    uint64_t temp = AES_INVSHIFROWS_LO(rs1,rs2);
    return  ((uint64_t)AES_DEC_SBOX[(temp >>  0) & 0xFF] <<  0) |
            ((uint64_t)AES_DEC_SBOX[(temp >>  8) & 0xFF] <<  8) |
            ((uint64_t)AES_DEC_SBOX[(temp >> 16) & 0xFF] << 16) |
            ((uint64_t)AES_DEC_SBOX[(temp >> 24) & 0xFF] << 24) |
            ((uint64_t)AES_DEC_SBOX[(temp >> 32) & 0xFF] << 32) |
            ((uint64_t)AES_DEC_SBOX[(temp >> 40) & 0xFF] << 40) |
            ((uint64_t)AES_DEC_SBOX[(temp >> 48) & 0xFF] << 48) |
            ((uint64_t)AES_DEC_SBOX[(temp >> 56) & 0xFF] << 56) ;
}

int64_t aes64dsm (int64_t rs1, int64_t rs2)
{
    uint64_t temp = aes64ds(rs1, rs2);
    uint32_t col_0 = temp & 0xFFFFFFFF;
    uint32_t col_1 = temp >> 32;
    col_0 = AES_INVMIXCOLUMN(col_0);
    col_1 = AES_INVMIXCOLUMN(col_1);
    return ((uint64_t)col_1 << 32) | col_0;
}

int64_t aes64im (int64_t rs1)
{
    uint32_t col_0 = rs1 & 0xFFFFFFFF;
    uint32_t col_1 = rs1 >> 32;
    col_0 = AES_INVMIXCOLUMN(col_0);
    col_1 = AES_INVMIXCOLUMN(col_1);
    return ((uint64_t)col_1 << 32) | col_0;
}

int64_t aes64ks1i (int64_t rs1, int64_t rs2)
{
    uint8_t     round_consts [10] = {
        0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36
    };
    
    uint32_t temp = (rs1 >> 32) & 0xFFFFFFFF;
    uint8_t enc_rcon = rs2 & 0xF;
    uint8_t rcon = 0;

    if(enc_rcon != 0xA) {
        temp = (temp >> 8) | (temp << 24); // Rotate left by 8
        rcon = round_consts[enc_rcon];
    }

    temp =  ((uint32_t)AES_ENC_SBOX[(temp >> 24) & 0xFF] << 24) |
            ((uint32_t)AES_ENC_SBOX[(temp >> 16) & 0xFF] << 16) |
            ((uint32_t)AES_ENC_SBOX[(temp >>  8) & 0xFF] <<  8) |
            ((uint32_t)AES_ENC_SBOX[(temp >>  0) & 0xFF] <<  0) ;
    
    temp ^= rcon;
    return ((uint64_t)temp << 32) | temp;
}

int64_t aes64ks2 (int64_t rs1, int64_t rs2)
{
    uint32_t lo = (rs1 >> 32) ^ rs2;
    uint32_t hi = (rs2 >> 32) ^ lo;
    return ((uint64_t)hi << 32) | lo;
}

int32_t sm4ed (int32_t rs1, int32_t rs2, uint8_t funct7)
{
    uint8_t bs = (funct7 >> 5) << 3;
    uint32_t sb_in = ((uint32_t)rs2 >> bs) & 0xFF;
    uint32_t sb_out = sm4_sbox[sb_in];

    uint32_t linear = sb_out ^  (sb_out         <<  8) ^ 
                                (sb_out         <<  2) ^
                                (sb_out         << 18) ^
                                ((sb_out & 0x3f) << 26) ^
                                ((sb_out & 0xC0) << 10) ;

    uint32_t rotl   = (linear << bs) | (linear >> (32-bs));

    return rotl ^ rs1;
}

int32_t sm4ks (int32_t rs1, int32_t rs2, uint8_t funct7)
{
    uint8_t bs = (funct7 >> 5) << 3;
    uint32_t sb_in = ((uint32_t)rs2 >> bs) & 0xFF;
    uint32_t sb_out = sm4_sbox[sb_in];

    uint32_t linear = sb_out ^ ((sb_out & 0x07) << 29) ^
                               ((sb_out & 0xFE) <<  7) ^
                               ((sb_out & 0x01) << 23) ^
                               ((sb_out & 0xF8) << 13) ;
                
    uint32_t rotl = (linear << bs) | (linear >> (32-bs));

    return rotl ^ rs1;
}

#endif