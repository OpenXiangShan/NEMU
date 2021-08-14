#ifndef __ICS_EXPORT
// This file is only used when exporting to ICS
#endif
def_EHelper(auipc) {
  rtl_li(s, ddest, id_src1->imm + s->pc);
}
