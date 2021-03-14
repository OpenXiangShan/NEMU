def_EHelper(lr_w) {
  rtl_lms(s, ddest, dsrc1, 0, 4);
  cpu.lr_addr = *dsrc1;
}

def_EHelper(lr_d) {
  rtl_lms(s, ddest, dsrc1, 0, 8);
  cpu.lr_addr = *dsrc1;
}

def_EHelper(sc_w) {
  // should check overlapping instead of equality
  if (cpu.lr_addr == *dsrc1) {
    rtl_sm(s, dsrc1, 0, dsrc2, 4);
    rtl_li(s, ddest, 0);
  } else {
    rtl_li(s, ddest, 1);
  }
}

def_EHelper(sc_d) {
  // should check overlapping instead of equality
  if (cpu.lr_addr == *dsrc1) {
    rtl_sm(s, dsrc1, 0, dsrc2, 8);
    rtl_li(s, ddest, 0);
  } else {
    rtl_li(s, ddest, 1);
  }
}

#define def_AMO_EHelper(name, width, body) \
  def_EHelper(name) { \
    cpu.amo = true; \
    rtl_lms(s, s0, dsrc1, 0, width); \
    body(s, s1, s0, dsrc2); \
    rtl_sm(s, dsrc1, 0, s1, width); \
    rtl_mv(s, ddest, s0); \
    cpu.amo = false; \
  }

#define AMObody_swap(s, res, mdata, rdata) rtl_mv (s, res, rdata)
#define AMObody_add(s, res, mdata, rdata)  rtl_add(s, res, mdata, rdata)
#define AMObody_or(s, res, mdata, rdata)   rtl_or (s, res, mdata, rdata)
#define AMObody_and(s, res, mdata, rdata)  rtl_and(s, res, mdata, rdata)
#define AMObody_xor(s, res, mdata, rdata)  rtl_xor(s, res, mdata, rdata)
#define AMObody_maxu(s, res, mdata, rdata) \
  (*(res) = (*(mdata) > *(rdata) ? *(mdata) : *(rdata)))
#define AMObody_max(s, res, mdata, rdata) \
  (*(res) = ((sword_t)*(mdata) > (sword_t)*(rdata) ? *(mdata) : *(rdata)))
#define AMObody_minu(s, res, mdata, rdata) \
  (*(res) = (*(mdata) < *(rdata) ? *(mdata) : *(rdata)))
#define AMObody_min(s, res, mdata, rdata) \
  (*(res) = ((sword_t)*(mdata) < (sword_t)*(rdata) ? *(mdata) : *(rdata)))

def_AMO_EHelper(amoswap_w, 4, AMObody_swap)
def_AMO_EHelper(amoadd_w , 4, AMObody_add)
def_AMO_EHelper(amoor_w  , 4, AMObody_or)
def_AMO_EHelper(amoand_w , 4, AMObody_and)
def_AMO_EHelper(amoxor_w , 4, AMObody_xor)
def_AMO_EHelper(amomaxu_w, 4, AMObody_maxu)
def_AMO_EHelper(amomax_w , 4, AMObody_max)
def_AMO_EHelper(amominu_w, 4, AMObody_minu)
def_AMO_EHelper(amomin_w , 4, AMObody_min)

def_AMO_EHelper(amoswap_d, 8, AMObody_swap)
def_AMO_EHelper(amoadd_d , 8, AMObody_add)
def_AMO_EHelper(amoor_d  , 8, AMObody_or)
def_AMO_EHelper(amoand_d , 8, AMObody_and)
def_AMO_EHelper(amoxor_d , 8, AMObody_xor)
def_AMO_EHelper(amomaxu_d, 8, AMObody_maxu)
def_AMO_EHelper(amomax_d , 8, AMObody_max)
def_AMO_EHelper(amominu_d, 8, AMObody_minu)
def_AMO_EHelper(amomin_d , 8, AMObody_min)
