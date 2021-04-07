def_DEWBWHelper(addl_G2E ,  G2E, add, E, l);
def_DEWBWHelper(addw_G2E ,  G2E, add, E, w);
def_DEWBWHelper(addl_E2G ,  E2G, add, r, l);
def_DEWBWHelper(addw_E2G ,  E2G, add, r, w);
def_DEWBWHelper(addl_SI2E,    E, add, E, l);
def_DEWBWHelper(addw_SI2E,    E, add, E, w);

def_DEWBWHelper(subl_E2G ,  E2G, sub, r, l);
def_DEWBWHelper(subw_E2G ,  E2G, sub, r, w);
def_DEWBWHelper(subl_SI2E,    E, sub, E, l);
def_DEWBWHelper(subw_SI2E,    E, sub, E, w);

def_DEWBWHelper(adcl_E2G ,  E2G, adc, r, l);
def_DEWBWHelper(adcw_E2G ,  E2G, adc, r, w);

def_DEWBWHelper(sbbl_E2G ,  E2G, sbb, r, l);
def_DEWBWHelper(sbbw_E2G ,  E2G, sbb, r, w);

def_DEWHelper(cmpl_G2E ,  G2E, cmp, l);
def_DEWHelper(cmpw_G2E ,  G2E, cmp, w);
def_DEWHelper(cmpb_G2E ,  G2E, cmp, b);
def_DEWHelper(cmpl_E2G ,  E2G, cmp, l);
def_DEWHelper(cmpw_E2G ,  E2G, cmp, w);
def_DEWHelper(cmpl_I2a ,    r, cmp, l);
def_DEWHelper(cmpw_I2a ,    r, cmp, w);
def_DEWHelper(cmpb_I2a ,    r, cmp, b);
def_DEWHelper(cmpl_I2E ,    E, cmp, l);
def_DEWHelper(cmpw_I2E ,    E, cmp, w);
def_DEWHelper(cmpb_I2E ,    E, cmp, b);
def_DEWHelper(cmpl_SI2E,    E, cmp, l);
def_DEWHelper(cmpw_SI2E,    E, cmp, w);

def_DEWBWHelper(incl_r, r, inc, r, l);
def_DEWBWHelper(incw_r, r, inc, r, w);
def_DEWBWHelper(incl_E, E, inc, E, l);
def_DEWBWHelper(incw_E, E, inc, E, w);

def_DEWBWHelper(decl_r, r, dec, r, l);
def_DEWBWHelper(decw_r, r, dec, r, w);

def_DEWBWHelper(imull_E2G, E2G, imul2, r, l);
def_DEWBWHelper(imulw_E2G, E2G, imul2, r, w);

def_DEWHelper(idivl_E, E, idiv, l);
def_DEWHelper(idivw_E, E, idiv, w);

#if 0
#ifndef __ICS_EXPORT

static inline def_EHelper(neg) {
#ifdef LAZY_CC
  rtl_sub(s, ddest, rz, ddest);
  rtl_set_lazycc(s, ddest, NULL, NULL, LAZYCC_NEG, id_dest->width);
  operand_write(s, id_dest, ddest);
#else
  rtl_sub(s, s0, rz, ddest);
  rtl_update_ZFSF(s, s0, id_dest->width);
  rtl_setrelopi(s, RELOP_NE, s1, ddest, 0);
  rtl_set_CF(s, s1);
  rtl_setrelopi(s, RELOP_EQ, s1, ddest, 0x1u << (id_dest->width * 8 - 1));
  rtl_set_OF(s, s1);
  operand_write(s, id_dest, s0);
#endif
  print_asm_template1(neg);
}

static inline def_EHelper(xadd) {
  rtl_add(s, s0, ddest, dsrc1);
#ifdef LAZY_CC
  rtl_set_lazycc_src1(s, dsrc1);
  rtl_set_lazycc(s, s0, NULL, NULL, LAZYCC_ADD, id_dest->width);
#else
  rtl_update_ZFSF(s, s0, id_dest->width);
  if (id_dest->width != 4) {
    rtl_andi(s, s0, s0, 0xffffffffu >> ((4 - id_dest->width) * 8));
  }
  rtl_is_add_carry(s, s1, s0, ddest);
  rtl_set_CF(s, s1);
  rtl_is_add_overflow(s, s1, s0, ddest, dsrc1, id_dest->width);
  rtl_set_OF(s, s1);
#endif
  operand_write(s, id_src1, ddest);
  operand_write(s, id_dest, s0);
  print_asm_template2(xadd);
}
#else
static inline def_EHelper(add) {
  TODO();
  print_asm_template2(add);
}

static inline def_EHelper(sub) {
  TODO();
  print_asm_template2(sub);
}

static inline def_EHelper(cmp) {
  TODO();
  print_asm_template2(cmp);
}

static inline def_EHelper(inc) {
  TODO();
  print_asm_template1(inc);
}

static inline def_EHelper(dec) {
  TODO();
  print_asm_template1(dec);
}

static inline def_EHelper(neg) {
  TODO();
  print_asm_template1(neg);
}
#endif


static inline def_EHelper(mul) {
  switch (id_dest->width) {
    case 1:
      rtl_lr(s, s0, R_EAX, 1);
      rtl_mulu_lo(s, s1, ddest, s0);
#ifndef __PA__
      rtl_update_ZFSF(s, s1, id_dest->width);
      rtl_andi(s, s0, s1, 0xff00);
      rtl_setrelopi(s, RELOP_NE, s0, s0, 0);
      rtl_set_OF(s, s0);
      rtl_set_CF(s, s0);
#endif
      rtl_sr(s, R_AX, s1, 2);
      break;
    case 2:
      rtl_lr(s, s0, R_EAX, 2);
      rtl_mulu_lo(s, s1, ddest, s0);
#ifndef __PA__
      rtl_update_ZFSF(s, s1, id_dest->width);
      rtl_shri(s, s0, s1, 16);
      rtl_setrelopi(s, RELOP_NE, s0, s0, 0);
      rtl_set_OF(s, s0);
      rtl_set_CF(s, s0);
#endif
      rtl_sr(s, R_AX, s1, 2);
      rtl_shri(s, s1, s1, 16);
      rtl_sr(s, R_DX, s1, 2);
      break;
    case 4:
      ; rtlreg_t *pdest = ddest;
      if (ddest == &cpu.edx) {
        rtl_mv(s, s0, ddest);
        pdest = s0;
      }
      rtl_mulu_hi(s, &cpu.edx, pdest, &cpu.eax);
      rtl_mulu_lo(s, &cpu.eax, pdest, &cpu.eax);
#ifndef __PA__
      rtl_update_ZFSF(s, &cpu.eax, id_dest->width);
      rtl_setrelopi(s, RELOP_NE, s0, &cpu.edx, 0);
      rtl_set_OF(s, s0);
      rtl_set_CF(s, s0);
#endif
      break;
    default: assert(0);
  }

  print_asm_template1(mul);
}

// imul with one operand
static inline def_EHelper(imul1) {
  switch (id_dest->width) {
    case 1:
      rtl_lr(s, s0, R_EAX, 1);
      rtl_sext(s, s0, s0, 1);
      rtl_sext(s, ddest, ddest, 1);
      rtl_mulu_lo(s, s1, ddest, s0);
#ifndef __PA__
      rtl_update_ZFSF(s, s1, 1);
      rtl_sext(s, s0, s1, 1);
      rtl_setrelop(s, RELOP_NE, s0, s0, s1);
#endif
      rtl_sr(s, R_AX, s1, 2);
      break;
    case 2:
      rtl_lr(s, s0, R_EAX, 2);
      rtl_sext(s, s0, s0, 2);
      rtl_sext(s, ddest, ddest, 2);
      rtl_mulu_lo(s, s1, ddest, s0);
#ifndef __PA__
      rtl_update_ZFSF(s, s1, 2);
      rtl_sext(s, s0, s1, 2);
      rtl_setrelop(s, RELOP_NE, s0, s0, s1);
#endif
      rtl_sr(s, R_AX, s1, 2);
      rtl_shri(s, s1, s1, 16);
      rtl_sr(s, R_DX, s1, 2);
      break;
    case 4:
      ; rtlreg_t *pdest = ddest;
      if (ddest == &cpu.edx) {
        rtl_mv(s, s0, ddest);
        pdest = s0;
      }
      rtl_muls_hi(s, &cpu.edx, pdest, &cpu.eax);
      rtl_mulu_lo(s, &cpu.eax, pdest, &cpu.eax);
#ifndef __PA__
      rtl_update_ZFSF(s, &cpu.eax, 4);
      rtl_msb(s, s0, &cpu.eax, 4);
      rtl_add(s, s0, &cpu.edx, s0);
      rtl_setrelopi(s, RELOP_NE, s0, s0, 0);
#endif
      break;
    default: assert(0);
  }

  rtl_set_CF(s, s0);
  rtl_set_OF(s, s0);
  print_asm_template1(imul);
}

// imul with three operands
static inline def_EHelper(imul3) {
  rtl_sext(s, dsrc2, dsrc2, id_dest->width);

#if !defined(__PA__) && defined(CONFIG_DIFFTEST)
  if (id_dest->width == 4) {
    rtl_muls_hi(s, s1, dsrc2, dsrc1);
  }
#endif

  rtl_mulu_lo(s, ddest, dsrc2, dsrc1);

#if !defined(__PA__) && defined(CONFIG_DIFFTEST)
  if (id_dest->width == 2) {
    rtl_sext(s, s0, ddest, id_dest->width);
    rtl_setrelop(s, RELOP_NE, s0, s0, ddest);
  } else if (id_dest->width == 4) {
    rtl_msb(s, s0, ddest, id_dest->width);
    rtl_add(s, s0, s1, s0);
    rtl_setrelopi(s, RELOP_NE, s0, s0, 0);
  } else {
    assert(0);
  }
  rtl_set_CF(s, s0);
  rtl_set_OF(s, s0);
  rtl_update_ZFSF(s, ddest, id_dest->width);
#endif
  operand_write(s, id_dest, ddest);

  print_asm_template3(imul);
}

static inline def_EHelper(div) {
  switch (id_dest->width) {
    case 1:
      rtl_lr(s, s0, R_AX, 2);
      rtl_divu_q(s, s1, s0, ddest);
      rtl_sr(s, R_AL, s1, 1);
      rtl_divu_r(s, s1, s0, ddest);
      rtl_sr(s, R_AH, s1, 1);
      break;
    case 2:
      rtl_lr(s, s0, R_AX, 2);
      rtl_lr(s, s1, R_DX, 2);
      rtl_shli(s, s1, s1, 16);
      rtl_or(s, s0, s0, s1);
      rtl_divu_q(s, s1, s0, ddest);
      rtl_sr(s, R_AX, s1, 2);
      rtl_divu_r(s, s1, s0, ddest);
      rtl_sr(s, R_DX, s1, 2);
      break;
    case 4:
      ; rtlreg_t *pdest = ddest;
      if (ddest == &cpu.eax) pdest = s0;
      rtl_mv(s, s0, &cpu.eax);
      rtl_div64u_q(s, &cpu.eax, &cpu.edx, s0, pdest);
      rtl_div64u_r(s, &cpu.edx, &cpu.edx, s0, pdest);
      break;
    default: assert(0);
  }

  print_asm_template1(div);
}

#endif
