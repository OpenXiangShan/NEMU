def_EHelper(bsr) {
#ifndef CONFIG_ENGINE_INTERPRETER
  panic("not support in engines other than interpreter");
#endif

  rtl_decode_binary(s, false, true);

  rtl_setrelopi(s, RELOP_EQ, s0, dsrc1, 0);
  rtl_set_ZF(s, s0);

  int bit = 31;
  if (*dsrc1 != 0) {
    while ((*dsrc1 & (1u << bit)) == 0) bit--;
    *ddest = bit;
    rtl_wb_r(s, ddest);
  }
}

def_EHelper(bt) {
  if (id_dest->type == OP_TYPE_MEM) {
    rtl_srli(s, s0, dsrc1, 5);
    rtl_slli(s, s0, s0, 2);
    rtl_add(s, s0, s->isa.mbase, s0);
    s->isa.mbase = s0;
  }

  rtl_decode_binary(s, true, true);

  rtl_li(s, s0, 1);
  rtl_sll(s, s0, s0, dsrc1);
  rtl_and(s, s0, s0, ddest);

#ifdef CONFIG_x86_CC_LAZY
  if (s->isa.flag_def != 0) {
    rtl_set_lazycc(s, s0, NULL, NULL, LAZYCC_BT, s->isa.width);
  }
#else
  int need_update_eflags = MUXDEF(CONFIG_x86_CC_SKIP, s->isa.flag_def != 0, true);
  if (need_update_eflags) {
    rtl_setrelopi(s, RELOP_NE, s0, s0, 0);
    rtl_set_CF(s, s0);
  }
#endif
}

def_EHelper(bsf) {
#ifndef CONFIG_ENGINE_INTERPRETER
  panic("not support in engines other than interpreter");
#endif

  rtl_decode_binary(s, false, true);

  rtl_setrelopi(s, RELOP_EQ, s0, dsrc1, 0);
  rtl_set_ZF(s, s0);
  rtl_set_CF(s, rz);

  int bit = 0;
  if (*dsrc1 != 0) {
    while ((*dsrc1 & (1u << bit)) == 0) bit++;
    *ddest = bit;
  } else if (s->isa.rep_flags == PREFIX_REP) {
    *ddest = 32;
  }
  rtl_wb_r(s, ddest);
}

def_EHelper(bts) {
  rtl_decode_binary(s, true, true);
  if (id_dest->type == OP_TYPE_MEM) { assert(0); }

  rtl_li(s, s0, 1);
  rtl_sll(s, s0, s0, dsrc1);
  rtl_and(s, s1, s0, ddest);
  rtl_setrelopi(s, RELOP_NE, s1, s1, 0);
  rtl_set_CF(s, s1);
  rtl_or(s, ddest, ddest, s0);
  rtl_wb_r(s, ddest);
}

#if 0
static inline def_EHelper(btc) {
  rtl_li(s, s0, 1);
  rtl_sll(s, s0, s0, dsrc1);
  rtl_and(s, s1, s0, ddest);
  rtl_setrelopi(s, RELOP_NE, s1, s1, 0);
  rtl_set_CF(s, s1);
  rtl_xor(s, ddest, ddest, s0);
  operand_write(s, id_dest, ddest);
}

static inline def_EHelper(btr) {
  rtl_li(s, s0, 1);
  rtl_sll(s, s0, s0, dsrc1);
  rtl_and(s, s1, s0, ddest);
  rtl_setrelopi(s, RELOP_NE, s1, s1, 0);
  rtl_set_CF(s, s1);
  rtl_not(s, s0, s0);
  rtl_and(s, ddest, ddest, s0);
  operand_write(s, id_dest, ddest);
}
#endif

def_EHelper(bswap) {
  // src[7:0]
  rtl_slli(s, s1, ddest, 24);

  // src[31:24]
  rtl_srli(s, s0, ddest, 24);
  rtl_or(s, s1, s1, s0);

  // src[15:8]
  rtl_slli(s, s0, ddest, 8);
  rtl_andi(s, s0, s0, 0xff0000);
  rtl_or(s, s1, s1, s0);

  // src[23:16]
  rtl_srli(s, s0, ddest, 8);
  rtl_andi(s, s0, s0, 0xff00);
  rtl_or(s, ddest, s1, s0);

  rtl_wb_r(s, ddest);
}
