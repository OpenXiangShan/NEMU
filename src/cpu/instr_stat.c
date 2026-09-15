#include <cpu/instr_stat.h>
#include <generated/autoconf.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#ifdef CONFIG_INSTR_CNT_BY_CATEGORY
#include <isa-all-instr.h>
#endif

const char *instr_stat_category_name(InstrStatCategory category) {
  static const char * const names[INSTR_STAT_NR] = {
    [INSTR_STAT_ALU] = "alu",
    [INSTR_STAT_LOAD] = "load",
    [INSTR_STAT_STORE] = "store",
    [INSTR_STAT_BRANCH] = "branch",
    [INSTR_STAT_JUMP] = "jump",
    [INSTR_STAT_CSR_SYSTEM] = "csr_system",
    [INSTR_STAT_FP] = "fp",
    [INSTR_STAT_VECTOR] = "vector",
    [INSTR_STAT_AMO] = "amo",
    [INSTR_STAT_FENCE] = "fence",
    [INSTR_STAT_OTHER] = "other",
  };

  if ((unsigned)category >= INSTR_STAT_NR) {
    return "unknown";
  }
  return names[category];
}

#ifdef CONFIG_INSTR_CNT_BY_CATEGORY

static uint64_t instr_stat_counts[INSTR_STAT_NR];

static const InstrStatCategory instr_stat_print_order[] = {
  INSTR_STAT_ALU,
  INSTR_STAT_LOAD,
  INSTR_STAT_STORE,
  INSTR_STAT_BRANCH,
  INSTR_STAT_JUMP,
  INSTR_STAT_CSR_SYSTEM,
  INSTR_STAT_FP,
  INSTR_STAT_VECTOR,
  INSTR_STAT_AMO,
  INSTR_STAT_FENCE,
  INSTR_STAT_OTHER,
};

#if defined(CONFIG_ISA_riscv64)

#define RV64_ALU_INSTR(f) \
  f(lui) f(auipc) \
  f(add) f(sll) f(srl) f(slt) f(sltu) f(xor) f(or) f(sub) f(sra) f(and) \
  f(addi) f(slli) f(srli) f(slti) f(sltui) f(xori) f(ori) f(srai) f(andi) \
  f(addw) f(sllw) f(srlw) f(subw) f(sraw) \
  f(addiw) f(slliw) f(srliw) f(sraiw) \
  f(mul) f(mulh) f(mulhu) f(mulhsu) f(div) f(divu) f(rem) f(remu) \
  f(mulw) f(divw) f(divuw) f(remw) f(remuw) \
  f(c_li) f(c_addi) f(c_slli) f(c_srli) f(c_srai) f(c_andi) f(c_addiw) \
  f(c_add) f(c_and) f(c_or) f(c_xor) f(c_sub) f(c_addw) f(c_subw) \
  f(c_mv) f(p_li_0) f(p_li_1) f(p_sext_w) f(p_inc) f(p_dec) \
  BITMANIP_INSTR_BINARY(f) \
  BITMANIP_INSTR_TERNARY(f) \
  CRYPTO_INSTR_BINARY(f) \
  CRYPTO_INSTR_TERNARY(f) \
  ZICOND_INSTR_TERNARY(f) \
  ZCB_INSTR_BINARY(f) \
  ZCB_INSTR_TERNARY(f)

#define RV64_LOAD_INSTR(f) \
  f(ld) f(lw) f(lh) f(lb) f(lwu) f(lhu) f(lbu) \
  f(ld_mmu) f(lw_mmu) f(lh_mmu) f(lb_mmu) f(lwu_mmu) f(lhu_mmu) f(lbu_mmu)

#define RV64_STORE_INSTR(f) \
  f(sd) f(sw) f(sh) f(sb) \
  f(sd_mmu) f(sw_mmu) f(sh_mmu) f(sb_mmu)

#define RV64_BRANCH_INSTR(f) \
  f(c_beqz) f(c_bnez) \
  f(beq) f(bne) f(blt) f(bge) f(bltu) f(bgeu) \
  f(p_blez) f(p_bgez) f(p_bltz) f(p_bgtz)

#define RV64_JUMP_INSTR(f) \
  f(jal) f(jalr) f(c_j) f(p_jal) f(c_jr) f(c_jalr) f(p_ret)

#define RV64_CSR_SYSTEM_INSTR(f) \
  SYS_INSTR_NULLARY(f) \
  SYS_INSTR_BINARY(f) \
  SYS_INSTR_TERNARY(f) \
  SYS_INSTR_TERNARY_CSR(f)

#define RV64_FP_INSTR(f) \
  FLOAT_INSTR_BINARY(f) \
  FLOAT_INSTR_TERNARY(f) \
  ZFH_MIN_INSTR_BINARY(f) \
  ZFH_INSTR_BINARY(f) \
  ZFH_INSTR_TERNARY(f) \
  ZFA_INSTR_BINARY(f) \
  ZFA_INSTR_TERNARY(f) \
  ZFBF_MIN_INSTR_BINARY(f) \
  ZFH_ZFA_INSTR_BINARY(f) \
  ZFH_ZFA_INSTR_TERNARY(f)

#define RV64_VECTOR_INSTR(f) \
  VECTOR_INSTR_TERNARY(f) \
  ZVFBF_MIN_INSTR_TERNARY(f) \
  ZVFBF_WMA_INSTR_TERNARY(f) \
  TENSOR_INSTR_BINARY(f) \
  TENSOR_INSTR_TERNARY(f)

#define RV64_AMO_INSTR(f) \
  AMO_INSTR_BINARY(f) \
  AMO_INSTR_TERNARY(f) \
  AMO_CAS_INSTR(f)

#define RV64_FENCE_INSTR(f) \
  f(fence_i) f(fence) \
  ZIHINTPAUSE_INSTR_NULLARY(f) \
  ZAWRS_INSTR_NULLARY(f) \
  CBO_INSTR_TERNARY(f)

#define INSTR_STAT_CASE(name) case EXEC_ID_##name:

InstrStatCategory instr_stat_category(int exec_id) {
  switch (exec_id) {
    RV64_ALU_INSTR(INSTR_STAT_CASE)
      return INSTR_STAT_ALU;
    RV64_LOAD_INSTR(INSTR_STAT_CASE)
      return INSTR_STAT_LOAD;
    RV64_STORE_INSTR(INSTR_STAT_CASE)
      return INSTR_STAT_STORE;
    RV64_BRANCH_INSTR(INSTR_STAT_CASE)
      return INSTR_STAT_BRANCH;
    RV64_JUMP_INSTR(INSTR_STAT_CASE)
      return INSTR_STAT_JUMP;
    RV64_CSR_SYSTEM_INSTR(INSTR_STAT_CASE)
      return INSTR_STAT_CSR_SYSTEM;
    RV64_FP_INSTR(INSTR_STAT_CASE)
      return INSTR_STAT_FP;
    RV64_VECTOR_INSTR(INSTR_STAT_CASE)
      return INSTR_STAT_VECTOR;
    RV64_AMO_INSTR(INSTR_STAT_CASE)
      return INSTR_STAT_AMO;
    RV64_FENCE_INSTR(INSTR_STAT_CASE)
      return INSTR_STAT_FENCE;
    default:
      return INSTR_STAT_OTHER;
  }
}

#undef INSTR_STAT_CASE

#else

InstrStatCategory instr_stat_category(int exec_id) {
  (void)exec_id;
  return INSTR_STAT_OTHER;
}

#endif

void instr_stat_count(InstrStatCategory category) {
  if ((unsigned)category >= INSTR_STAT_NR) {
    category = INSTR_STAT_OTHER;
  }
  instr_stat_counts[category]++;
}

uint64_t instr_stat_get(InstrStatCategory category) {
  if ((unsigned)category >= INSTR_STAT_NR) {
    return 0;
  }
  return instr_stat_counts[category];
}

void instr_stat_reset(void) {
  memset(instr_stat_counts, 0, sizeof(instr_stat_counts));
}

int instr_stat_format(char *buf, size_t size) {
  int offset = 0;
  int written = snprintf(buf, size, "instruction category statistics:\n");
  if (written < 0) {
    return written;
  }
  offset += written;

  for (int i = 0; i < ARRLEN(instr_stat_print_order); i++) {
    InstrStatCategory category = instr_stat_print_order[i];
    size_t remain = offset < (int)size ? size - offset : 0;
    written = snprintf(buf + offset, remain, "  %-10s = %" PRIu64 "\n",
                       instr_stat_category_name(category), instr_stat_get(category));
    if (written < 0) {
      return written;
    }
    offset += written;
  }

  return offset;
}

#else

InstrStatCategory instr_stat_category(int exec_id) {
  (void)exec_id;
  return INSTR_STAT_OTHER;
}

void instr_stat_count(InstrStatCategory category) {
  (void)category;
}

uint64_t instr_stat_get(InstrStatCategory category) {
  (void)category;
  return 0;
}

void instr_stat_reset(void) {
}

int instr_stat_format(char *buf, size_t size) {
  if (size > 0) {
    buf[0] = '\0';
  }
  return 0;
}

#endif
