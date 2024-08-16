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

#include <stdint.h>

extern const uint8_t AES_ENC_SBOX[];
extern const uint8_t AES_DEC_SBOX[];

#define AES_UNPACK_BYTES(b0,b1,b2,b3) \
    uint8_t  b0 = (RS1 >>  0) & 0xFF; \
    uint8_t  b1 = (RS2 >>  8) & 0xFF; \
    uint8_t  b2 = (RS1 >> 16) & 0xFF; \
    uint8_t  b3 = (RS2 >> 24) & 0xFF; \

#define AES_PACK_BYTES(b0,b1,b2,b3) ( \
    (uint32_t)b0 <<  0  | \
    (uint32_t)b1 <<  8  | \
    (uint32_t)b2 << 16  | \
    (uint32_t)b3 << 24  )

#define AES_SBOX(b0, b1, b2, b3) \
    b0 = AES_ENC_SBOX[b0]; \
    b1 = AES_ENC_SBOX[b1]; \
    b2 = AES_ENC_SBOX[b2]; \
    b3 = AES_ENC_SBOX[b3]; \

#define AES_RSBOX(b0, b1, b2, b3) \
    b0 = AES_DEC_SBOX[b0]; \
    b1 = AES_DEC_SBOX[b1]; \
    b2 = AES_DEC_SBOX[b2]; \
    b3 = AES_DEC_SBOX[b3]; \

#define AES_XTIME(a) \
    ((a << 1) ^ ((a&0x80) ? 0x1b : 0))

#define AES_GFMUL(a,b) (( \
    ( ( (b) & 0x1 ) ?                              (a)   : 0 ) ^ \
    ( ( (b) & 0x2 ) ?                     AES_XTIME(a)   : 0 ) ^ \
    ( ( (b) & 0x4 ) ?           AES_XTIME(AES_XTIME(a))  : 0 ) ^ \
    ( ( (b) & 0x8 ) ? AES_XTIME(AES_XTIME(AES_XTIME(a))) : 0 ) )&0xFF)

#define BY(X,I) ((X >> (8*I)) & 0xFF)

#define AES_SHIFROWS_LO(RS1,RS2) ( \
    (((RS1 >> 24) & 0xFF) << 56) | \
    (((RS2 >> 48) & 0xFF) << 48) | \
    (((RS2 >>  8) & 0xFF) << 40) | \
    (((RS1 >> 32) & 0xFF) << 32) | \
    (((RS2 >> 56) & 0xFF) << 24) | \
    (((RS2 >> 16) & 0xFF) << 16) | \
    (((RS1 >> 40) & 0xFF) <<  8) | \
    (((RS1 >>  0) & 0xFF) <<  0) ) 

#define AES_INVSHIFROWS_LO(RS1,RS2) ( \
    (((RS2 >> 24) & 0xFF) << 56) | \
    (((RS2 >> 48) & 0xFF) << 48) | \
    (((RS1 >>  8) & 0xFF) << 40) | \
    (((RS1 >> 32) & 0xFF) << 32) | \
    (((RS1 >> 56) & 0xFF) << 24) | \
    (((RS2 >> 16) & 0xFF) << 16) | \
    (((RS2 >> 40) & 0xFF) <<  8) | \
    (((RS1 >>  0) & 0xFF) <<  0) ) 


#define AES_MIXBYTE(COL,B0,B1,B2,B3) ( \
              BY(COL,B3)     ^ \
              BY(COL,B2)     ^ \
    AES_GFMUL(BY(COL,B1), 3) ^ \
    AES_GFMUL(BY(COL,B0), 2)   \
)

#define AES_MIXCOLUMN(COL) ( \
    AES_MIXBYTE(COL,3,0,1,2) << 24 | \
    AES_MIXBYTE(COL,2,3,0,1) << 16 | \
    AES_MIXBYTE(COL,1,2,3,0) <<  8 | \
    AES_MIXBYTE(COL,0,1,2,3) <<  0   \
)


#define AES_INVMIXBYTE(COL,B0,B1,B2,B3) ( \
    AES_GFMUL(BY(COL,B3),0x9) ^ \
    AES_GFMUL(BY(COL,B2),0xd) ^ \
    AES_GFMUL(BY(COL,B1),0xb) ^ \
    AES_GFMUL(BY(COL,B0),0xe)   \
)

#define AES_INVMIXCOLUMN(COL) ( \
    AES_INVMIXBYTE(COL,3,0,1,2) << 24 | \
    AES_INVMIXBYTE(COL,2,3,0,1) << 16 | \
    AES_INVMIXBYTE(COL,1,2,3,0) <<  8 | \
    AES_INVMIXBYTE(COL,0,1,2,3) <<  0   \
)

