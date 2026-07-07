#include <cpu/instr_stat.h>
#include <generated/autoconf.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#ifdef CONFIG_INSTR_CATEGORY_STAT
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

#ifdef CONFIG_INSTR_CATEGORY_STAT

static uint64_t instr_stat_exec_counts[TOTAL_INSTR];
static uint64_t instr_stat_invalid_count;

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

void instr_stat_count(int exec_id) {
  if ((unsigned)exec_id >= TOTAL_INSTR) {
    instr_stat_invalid_count++;
    return;
  }
  instr_stat_exec_counts[exec_id]++;
}

#define INSTR_STAT_COUNT(name) + instr_stat_exec_counts[EXEC_ID_##name]
#define INSTR_STAT_SUM(list) (0 list(INSTR_STAT_COUNT))

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

static uint64_t instr_stat_known_count(void);

uint64_t instr_stat_get(InstrStatCategory category) {
  switch (category) {
    case INSTR_STAT_ALU: return INSTR_STAT_SUM(RV64_ALU_INSTR);
    case INSTR_STAT_LOAD: return INSTR_STAT_SUM(RV64_LOAD_INSTR);
    case INSTR_STAT_STORE: return INSTR_STAT_SUM(RV64_STORE_INSTR);
    case INSTR_STAT_BRANCH: return INSTR_STAT_SUM(RV64_BRANCH_INSTR);
    case INSTR_STAT_JUMP: return INSTR_STAT_SUM(RV64_JUMP_INSTR);
    case INSTR_STAT_CSR_SYSTEM: return INSTR_STAT_SUM(RV64_CSR_SYSTEM_INSTR);
    case INSTR_STAT_FP: return INSTR_STAT_SUM(RV64_FP_INSTR);
    case INSTR_STAT_VECTOR: return INSTR_STAT_SUM(RV64_VECTOR_INSTR);
    case INSTR_STAT_AMO: return INSTR_STAT_SUM(RV64_AMO_INSTR);
    case INSTR_STAT_FENCE: return INSTR_STAT_SUM(RV64_FENCE_INSTR);
    case INSTR_STAT_OTHER: {
      uint64_t total = instr_stat_invalid_count + INSTR_STAT_SUM(INSTR_LIST);
      uint64_t known = instr_stat_known_count();
      return total > known ? total - known : 0;
    }
    case INSTR_STAT_NR: break;
  }
  return 0;
}

static uint64_t instr_stat_known_count(void) {
  return instr_stat_get(INSTR_STAT_ALU) +
         instr_stat_get(INSTR_STAT_LOAD) +
         instr_stat_get(INSTR_STAT_STORE) +
         instr_stat_get(INSTR_STAT_BRANCH) +
         instr_stat_get(INSTR_STAT_JUMP) +
         instr_stat_get(INSTR_STAT_CSR_SYSTEM) +
         instr_stat_get(INSTR_STAT_FP) +
         instr_stat_get(INSTR_STAT_VECTOR) +
         instr_stat_get(INSTR_STAT_AMO) +
         instr_stat_get(INSTR_STAT_FENCE);
}

#else

uint64_t instr_stat_get(InstrStatCategory category) {
  if (category == INSTR_STAT_OTHER) {
    return instr_stat_invalid_count + INSTR_STAT_SUM(INSTR_LIST);
  }
  return 0;
}

#endif

void instr_stat_reset(void) {
  memset(instr_stat_exec_counts, 0, sizeof(instr_stat_exec_counts));
  instr_stat_invalid_count = 0;
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

void instr_stat_count(int exec_id) {
  (void)exec_id;
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
