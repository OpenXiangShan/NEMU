#define def_ld_template_with_trigger(name, rtl_instr, width, mmu_mode) \
  def_EHelper(name) { \
    trig_action_t action = TRIG_ACTION_NONE; \
    const vaddr_t vaddr = *dsrc1 + id_src2->imm; \
    if (cpu.TM->check_timings.br) { \
      action = tm_check_hit(cpu.TM, TRIG_OP_LOAD, vaddr, TRIGGER_NO_VALUE); \
    } \
    trigger_handler(action); \
    concat(rtl_, rtl_instr) (s, ddest, dsrc1, id_src2->imm, width, mmu_mode); \
    const rtlreg_t data = *ddest; \
    if (cpu.TM->check_timings.ar) { \
      action = tm_check_hit(cpu.TM, TRIG_OP_LOAD | TRIG_OP_TIMING, vaddr, data); \
    } \
    trigger_handler(action); \
  }

#define def_st_template_with_trigger(name, rtl_instr, width, mmu_mode) \
  def_EHelper(name) { \
    trig_action_t action = TRIG_ACTION_NONE; \
    const vaddr_t vaddr = *dsrc1 + id_src2->imm; \
    const word_t data = *ddest; \
    if (cpu.TM->check_timings.bw) { \
      action = tm_check_hit(cpu.TM, TRIG_OP_STORE, vaddr, data); \
    } \
    trigger_handler(action); \
    concat(rtl_, rtl_instr) (s, ddest, dsrc1, id_src2->imm, width, mmu_mode); \
  }

#define def_all_ldst(suffix, mmu_mode) \
  def_ld_template_with_trigger(concat(ld , suffix), lms, 8, mmu_mode) \
  def_ld_template_with_trigger(concat(lw , suffix), lms, 4, mmu_mode) \
  def_ld_template_with_trigger(concat(lh , suffix), lms, 2, mmu_mode) \
  def_ld_template_with_trigger(concat(lb , suffix), lms, 1, mmu_mode) \
  def_ld_template_with_trigger(concat(lwu, suffix), lm , 4, mmu_mode) \
  def_ld_template_with_trigger(concat(lhu, suffix), lm , 2, mmu_mode) \
  def_ld_template_with_trigger(concat(lbu, suffix), lm , 1, mmu_mode) \
  def_st_template_with_trigger(concat(sd , suffix), sm , 8, mmu_mode) \
  def_st_template_with_trigger(concat(sw , suffix), sm , 4, mmu_mode) \
  def_st_template_with_trigger(concat(sh , suffix), sm , 2, mmu_mode) \
  def_st_template_with_trigger(concat(sb , suffix), sm , 1, mmu_mode)

def_all_ldst(, MMU_DIRECT)
def_all_ldst(_mmu, MMU_TRANSLATE)
