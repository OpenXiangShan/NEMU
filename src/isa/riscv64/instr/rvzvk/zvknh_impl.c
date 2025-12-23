/***************************************************************************************
* Copyright (c) 2025 Institute of Computing Technology, Chinese Academy of Sciences
* Copyright (c) 2025 Beijing Institute of Open Source Chip
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

#include <cpu/decode.h>
#include <cpu/cpu.h>
#include <stdint.h>
#include "../rvv/vcommon.h"
#include "../rvv/vcompute_impl.h"
#include "../rvk/crypto_impl.h"


#define id_src1 (&s->src1)
#define id_src2 (&s->src2)
#define id_dest (&s->dest)

#define ch(x, y, z) ((x & y) ^ ((~x) & z))
#define maj(x, y, z) ((x & y) ^ (x & z) ^ (y & z))

void vsha2ms(Decode *s) {
  uint64_t sew = 8 << vtype->vsew;
  uint64_t egw = 4 * sew;
  uint64_t egs = 4;

  if (sew != 32 && sew != 64) {
    Log("vsha2ms: invalid sew %ld\n", sew);
    longjmp_exception(EX_II);
  }

  double vflmul = compute_vflmul();

  if (vflmul * VLEN < egw) {
    Log("vsha2ms: invalid vlmul %d for sew %ld\n", vtype->vlmul, sew);
    longjmp_exception(EX_II);
  }

  uint64_t w0, w1, w2, w3;
  uint64_t w4, w9, w10, w11;
  uint64_t w12, w13, w14, w15;
  uint64_t w16, w17, w18, w19;
  
  uint64_t eg_len = vl->val / egs;
  uint64_t eg_start = vstart->val / egs;

  for (int i = eg_start; i < eg_len; i++) {
    get_vreg(id_dest->reg, i * 4 + 0, &w3,  vtype->vsew, vtype->vlmul, 0, 0);
    get_vreg(id_dest->reg, i * 4 + 1, &w2,  vtype->vsew, vtype->vlmul, 0, 0);
    get_vreg(id_dest->reg, i * 4 + 2, &w1,  vtype->vsew, vtype->vlmul, 0, 0);
    get_vreg(id_dest->reg, i * 4 + 3, &w0,  vtype->vsew, vtype->vlmul, 0, 0);
    
    get_vreg(id_src2->reg, i * 4 + 0, &w11, vtype->vsew, vtype->vlmul, 0, 0);
    get_vreg(id_src2->reg, i * 4 + 1, &w10, vtype->vsew, vtype->vlmul, 0, 0);
    get_vreg(id_src2->reg, i * 4 + 2, &w9,  vtype->vsew, vtype->vlmul, 0, 0);
    get_vreg(id_src2->reg, i * 4 + 3, &w4,  vtype->vsew, vtype->vlmul, 0, 0);
    
    get_vreg(id_src1->reg, i * 4 + 0, &w15, vtype->vsew, vtype->vlmul, 0, 0);
    get_vreg(id_src1->reg, i * 4 + 1, &w14, vtype->vsew, vtype->vlmul, 0, 0);
    get_vreg(id_src1->reg, i * 4 + 2, &w13, vtype->vsew, vtype->vlmul, 0, 0);
    get_vreg(id_src1->reg, i * 4 + 3, &w12, vtype->vsew, vtype->vlmul, 0, 0);

    if (sew == 32) {
      w16 = (uint32_t) (sha256sig0(w14) + w9  + sha256sig0(w1) + w0);
      w17 = (uint32_t) (sha256sig0(w15) + w10 + sha256sig0(w2) + w1);
      w18 = (uint32_t) (sha256sig0(w16) + w11 + sha256sig0(w3) + w2);
      w19 = (uint32_t) (sha256sig0(w17) + w12 + sha256sig0(w4) + w3);
    } else if (sew == 64) {
      w16 = (uint64_t) (sha512sig0(w14) + w9  + sha512sig0(w1) + w0);
      w17 = (uint64_t) (sha512sig0(w15) + w10 + sha512sig0(w2) + w1);
      w18 = (uint64_t) (sha512sig0(w16) + w11 + sha512sig0(w3) + w2);
      w19 = (uint64_t) (sha512sig0(w17) + w12 + sha512sig0(w4) + w3);
    }

    set_vreg(id_dest->reg, i * 4 + 0, w19, vtype->vsew, vtype->vlmul, 0);
    set_vreg(id_dest->reg, i * 4 + 1, w18, vtype->vsew, vtype->vlmul, 0);
    set_vreg(id_dest->reg, i * 4 + 2, w17, vtype->vsew, vtype->vlmul, 0);
    set_vreg(id_dest->reg, i * 4 + 3, w16, vtype->vsew, vtype->vlmul, 0);
  }

}

void vsha2c(int is_vsha2cl, Decode *s) {
  uint64_t sew = 8 << vtype->vsew;
  uint64_t egw = 4 * sew;
  uint64_t egs = 4;

  if (sew != 32 && sew != 64) {
    Log("vsha2c: invalid sew %ld\n", sew);
    longjmp_exception(EX_II);
  }

  double vflmul = compute_vflmul();

  if (vflmul * VLEN < egw) {
    Log("vsha2c: invalid vlmul %d for sew %ld\n", vtype->vlmul, sew);
    longjmp_exception(EX_II);
  }

  uint64_t a, b, c, d;
  uint64_t e, f, g, h;
  uint64_t mspc0, mspc1, mspc2, mspc3;  // MessageSchedPlusC
  uint64_t w0, w1;
  uint64_t t1, t2;
  
  uint64_t eg_len = vl->val / egs;
  uint64_t eg_start = vstart->val / egs;

  for (int i = eg_start; i < eg_len; i++) {
    get_vreg(id_src2->reg, i * 4 + 0, &a, vtype->vsew, vtype->vlmul, 0, 0);
    get_vreg(id_src2->reg, i * 4 + 1, &b, vtype->vsew, vtype->vlmul, 0, 0);
    get_vreg(id_src2->reg, i * 4 + 2, &e, vtype->vsew, vtype->vlmul, 0, 0);
    get_vreg(id_src2->reg, i * 4 + 3, &f, vtype->vsew, vtype->vlmul, 0, 0);

    get_vreg(id_dest->reg, i * 4 + 0, &c,  vtype->vsew, vtype->vlmul, 0, 0);
    get_vreg(id_dest->reg, i * 4 + 1, &d,  vtype->vsew, vtype->vlmul, 0, 0);
    get_vreg(id_dest->reg, i * 4 + 2, &g,  vtype->vsew, vtype->vlmul, 0, 0);
    get_vreg(id_dest->reg, i * 4 + 3, &h,  vtype->vsew, vtype->vlmul, 0, 0);
    
    get_vreg(id_src1->reg, i * 4 + 0, &mspc3, vtype->vsew, vtype->vlmul, 0, 0);
    get_vreg(id_src1->reg, i * 4 + 1, &mspc2, vtype->vsew, vtype->vlmul, 0, 0);
    get_vreg(id_src1->reg, i * 4 + 2, &mspc1, vtype->vsew, vtype->vlmul, 0, 0);
    get_vreg(id_src1->reg, i * 4 + 3, &mspc0, vtype->vsew, vtype->vlmul, 0, 0);

    if (is_vsha2cl) {
      w1 = mspc1;
      w0 = mspc0;
    } else {
      w1 = mspc3;
      w0 = mspc2;
    }

    if (sew == 32) {
      t1 = (uint32_t) (h + sha256sum1(e) + ch(e, f, g) + w0);
      t2 = (uint32_t) (sha256sum0(a) + maj(a, b, c));
      h = g;
      g = f;
      f = e;
      e = (uint32_t)(d + t1);
      d = c;
      c = b;
      b = a;
      a = (uint32_t)(t1 + t2);

      t1 = (uint32_t) (h + sha256sum1(e) + ch(e, f, g) + w1);
      t2 = (uint32_t) (sha256sum0(a) + maj(a, b, c));
      h = g;
      g = f;
      f = e;
      e = (uint32_t)(d + t1);
      d = c;
      c = b;
      b = a;
      a = (uint32_t)(t1 + t2);
    } else if (sew == 64) {
      t1 = (uint64_t) (h + sha512sum1(e) + ch(e, f, g) + w0);
      t2 = (uint64_t) (sha512sum0(a) + maj(a, b, c));
      h = g;
      g = f;
      f = e;
      e = (uint64_t)(d + t1);
      d = c;
      c = b;
      b = a;
      a = (uint64_t)(t1 + t2);

      t1 = (uint64_t) (h + sha512sum1(e) + ch(e, f, g) + w1);
      t2 = (uint64_t) (sha512sum0(a) + maj(a, b, c));
      h = g;
      g = f;
      f = e;
      e = (uint64_t)(d + t1);
      d = c;
      c = b;
      b = a;
      a = (uint64_t)(t1 + t2);
    }

    set_vreg(id_dest->reg, i * 4 + 0, a, vtype->vsew, vtype->vlmul, 0);
    set_vreg(id_dest->reg, i * 4 + 1, b, vtype->vsew, vtype->vlmul, 0);
    set_vreg(id_dest->reg, i * 4 + 2, e, vtype->vsew, vtype->vlmul, 0);
    set_vreg(id_dest->reg, i * 4 + 3, f, vtype->vsew, vtype->vlmul, 0);
  }
}