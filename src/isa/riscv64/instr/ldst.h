#define def_ldst_template(name, rtl_instr, width, mmu_mode) \
def_EHelper(name) { \
  concat(rtl_, rtl_instr) (s, ddest, dsrc1, id_src2->imm, width, mmu_mode); \
}

def_ldst_template(ld, lms, 8, MMU_DYNAMIC)
def_ldst_template(lw, lms, 4, MMU_DYNAMIC)
def_ldst_template(lh, lms, 2, MMU_DYNAMIC)
def_ldst_template(lb, lms, 1, MMU_DYNAMIC)
def_ldst_template(lwu, lm, 4, MMU_DYNAMIC)
def_ldst_template(lhu, lm, 2, MMU_DYNAMIC)
def_ldst_template(lbu, lm, 1, MMU_DYNAMIC)
def_ldst_template(sd, sm,  8, MMU_DYNAMIC)
def_ldst_template(sw, sm,  4, MMU_DYNAMIC)
def_ldst_template(sh, sm,  2, MMU_DYNAMIC)
def_ldst_template(sb, sm,  1, MMU_DYNAMIC)
