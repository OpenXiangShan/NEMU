// Host-only regression of the canonical AME handlers, not a second model.
// Build explicitly with -DNEMU_MXFP_TEST; ordinary NEMU builds see an empty TU.
#ifdef NEMU_MXFP_TEST
#include <common.h>
#include <cpu/exec.h>
// Include the actual handlers as functions rather than interpreter labels.
#undef def_EHelper
#define def_EHelper(name) static inline void concat(exec_, name)(Decode *s)
#include <math.h>
#include <setjmp.h>
#include <stdlib.h>
#include <stddef.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <softfloat.h>
#include "mldst.h"
#include "mcompute.h"
#include "mcfg.h"

#ifdef NEMU_MXFP_SHARED_TEST
#if !defined(NEMU_MXFP_QUEUE_TEST) || !defined(CONFIG_RV_AME) || \
    !defined(CONFIG_SHARE) || !defined(CONFIG_SHARE_REF) || \
    !defined(CONFIG_DIFFTEST_AMU_CTRL) || defined(CONFIG_SHARE_CTRL) || \
    !defined(CONFIG_RV_AME_MXFP_CONVERT_LOAD) || \
    !defined(CONFIG_RV_AME_MXFP_DYNAMIC_QUANT) || !defined(CONFIG_FPU_SOFT)
#error "NEMU_MXFP_SHARED_TEST requires the admitted SHARE_REF dynamic fixture config"
#endif
#endif

CPU_state cpu;
rtlreg_t csr_array[4096], tmp_reg[4];
#define TEST_CSR_PTR(name, addr) name##_t *const name = (name##_t *)&csr_array[addr];
MAP(CSRS, TEST_CSR_PTR)
#undef TEST_CSR_PTR
const rtlreg_t rzero = 0;
Decode *prev_s;
#if defined(CONFIG_RV_AME_MXFP_CONVERT_LOAD) && defined(CONFIG_FPU_SOFT) && \
    !defined(CONFIG_SHARE) && !defined(CONFIG_SHARE_REF) && \
    !defined(CONFIG_SHARE_CTRL) && !defined(CONFIG_DIFFTEST_AMU_CTRL)
// Existing fixture memory, enlarged only for the opt-in 64-row F32 source.
// No allocator/runner: retain at least the old whole-register capacity.
static uint8_t memory[TLEN / 8 > 64 * 260 + 256 ?
  TLEN / 8 : 64 * 260 + 256];
#else
static uint8_t memory[TLEN / 8 > 4096 ? TLEN / 8 : 4096];
#endif
#ifdef NEMU_MXFP_QUEUE_TEST
#include <ext/amuctrl.h>
#include <ext/amu_ctrl_queue_wrapper.h>
_Static_assert(offsetof(amu_ctrl_event_t, pc) == 40, "legacy AMU prefix changed");
_Static_assert(offsetof(amu_ctrl_event_t, computeType) == 48, "MX type must follow pc");
_Static_assert(offsetof(amu_ctrl_event_t, isScale) == 49, "MX flag must follow type");
_Static_assert(offsetof(amu_ctrl_event_t, isConvertF32) == 50 &&
  offsetof(amu_ctrl_event_t, isDynamicMxfpQuant) == 51 &&
  sizeof(amu_ctrl_event_t) == 56, "v2 conversion selectors use original padding");
_Static_assert(sizeof(amu_ctrl_snapshot_t) == 128 &&
  offsetof(amu_ctrl_snapshot_t, expected_tr) == 96 &&
  offsetof(amu_ctrl_snapshot_t, raised_flags) == 120 &&
  offsetof(amu_ctrl_snapshot_t, reserved) == 124 &&
  sizeof(amu_mma_snapshot_t) == 120, "v2 bounded result descriptor ABI");
_Static_assert(sizeof(cpu.mcfg) == 64, "MCFG register ABI must contain eight u64 values");
_Static_assert(offsetof(CPU_state, mcfg) == offsetof(CPU_state, mcsr) + 13 * sizeof(uint64_t),
               "MCFG must follow the architectural matrix CSR block");
#ifdef CONFIG_DIFFTEST_CHECK_SDTRIG
_Static_assert(offsetof(CPU_state, tselect) == offsetof(CPU_state, mcfg) + sizeof(cpu.mcfg),
               "trigger state must follow MCFG in RefProxy register-copy order");
#endif
bool enable_fast_log, enable_small_log;
FILE *log_fp;
char *log_filebuf;
uint64_t record_row_number;
void log_buffer_flush(void) {}
static bool allow_matrix_memory;
uint8_t *guest_to_host(paddr_t address) {
  assert(allow_matrix_memory && address < sizeof(memory));
  return &memory[address];
}
void mstore_queue_update_mrelease(uint8_t token, uint64_t value) {
  (void)token; (void)value;
  assert(!"MX verification must fail before releasing a token");
  abort();
}
#endif
static jmp_buf exception_env;
static int exception_cause;
static size_t reads;
static vaddr_t fault_at = ~(vaddr_t)0;

void mp_set_dirty(void) {}
void isa_reg_display(void) {}
void monitor_statistic(void) {}
int isa_mmu_check(vaddr_t addr, int len, int type) {
  (void)addr; (void)len; (void)type;
  return MMU_DIRECT;
}
paddr_t isa_mmu_translate(vaddr_t addr, int len, int type) {
  (void)addr; (void)len; (void)type;
  assert(!"unexpected translation in direct-memory fixture");
  return 0;
}

__attribute__((noreturn)) void longjmp_exception(int cause) {
  exception_cause = cause;
  longjmp(exception_env, 1);
}

word_t vaddr_read(Decode *s, vaddr_t addr, int len, int mmu_mode) {
  (void)s; (void)mmu_mode;
  assert((len == 1 || len == 4) && addr <= sizeof(memory) - len);
  if (addr == fault_at) longjmp_exception(EX_LPF);
  reads++;
  word_t value = 0;
  for (int b = 0; b < len; ++b) value |= (word_t)memory[addr + b] << (8 * b);
  return value;
}

void vaddr_read_matrix(Decode *s, vaddr_t base, vaddr_t stride,
    int row, int column, int msew, bool transpose, int mmu_mode,
    char kind, int reg) {
  (void)s; (void)base; (void)stride; (void)row; (void)column;
  (void)msew; (void)transpose; (void)mmu_mode; (void)kind; (void)reg;
  assert(!"unexpected non-packed matrix load in this focused test");
}

void rtl_hostcall(Decode *s, uint32_t id, rtlreg_t *dest,
    const rtlreg_t *src1, const rtlreg_t *src2, word_t imm) {
  (void)s; (void)id; (void)dest; (void)src1; (void)src2; (void)imm;
  assert(!"unexpected standard floating-point hostcall");
}

static uint32_t float_bits(float value) {
  uint32_t bits;
  memcpy(&bits, &value, sizeof(bits));
  return bits;
}

// Exact integer dyadic-to-binary32 oracle. No production decoder is called.
static uint32_t dyadic_bits(unsigned significand, int power, bool negative) {
  uint32_t sign = negative ? 0x80000000u : 0;
  if (!significand) return sign;
  unsigned top = 0;
  for (unsigned v = significand; v >>= 1;) top++;
  int exponent = (int)top + power;
  if (exponent < -126) return sign | (significand << (power + 149));
  return sign | ((uint32_t)(exponent + 127) << 23) |
    ((significand << (23 - top)) & 0x7fffff);
}

static void test_formats(void) {
  static const uint32_t fp4_positive[8] = {
    0, 0x3f000000, 0x3f800000, 0x3fc00000,
    0x40000000, 0x40400000, 0x40800000, 0x40c00000
  };
  for (unsigned b = 0; b < 16; b++) {
    assert(float_bits(mxfp_decode_e2m1(b)) ==
      (fp4_positive[b & 7] | ((b & 8) ? 0x80000000u : 0)));
  }
  for (unsigned b = 0; b < 256; b++) {
    unsigned e4 = (b >> 3) & 15, m4 = b & 7;
    unsigned e5 = (b >> 2) & 31, m5 = b & 3;
    float v4 = mxfp_decode_e4m3(b), v5 = mxfp_decode_e5m2(b);
    if (e4 == 15 && m4 == 7) assert(isnan(v4));
    else assert(float_bits(v4) == dyadic_bits((e4 ? 8 : 0) + m4,
      e4 ? (int)e4 - 10 : -9, b >> 7));
    if (e5 == 31 && m5) assert(isnan(v5));
    else if (e5 == 31) assert(float_bits(v5) == ((b >> 7 ? 0x80000000u : 0) | 0x7f800000));
    else assert(float_bits(v5) == dyadic_bits((e5 ? 4 : 0) + m5,
      e5 ? (int)e5 - 17 : -16, b >> 7));
    float scale = mxfp_e8m0_to_float(b);
    if (b == 255) assert(isnan(scale));
    else assert(float_bits(scale) == (b ? b << 23 : 0x00400000));
  }
  assert(isnan(mxfp_decode_element(0, 255, 1, 0)));
  puts("PASS formats: E2M1=16 E4M3=256 E5M2=256 E8M0=256");
}

static unsigned supplied_blocks, supplied_lanes;

static void check_supplied_block(const uint32_t values[32], uint8_t e,
    bool fp4, bool e5m2, const uint8_t codes[32], bool accepted) {
  uint8_t guarded[34];
  uint32_t unchanged[32];
  memset(guarded, 0xa5, sizeof(guarded));
  memcpy(unchanged, values, sizeof(unchanged));
  const uint_fast8_t old_rm = softfloat_roundingMode;
  const uint_fast8_t old_flags = softfloat_exceptionFlags;
  const uint_fast8_t old_tininess = softfloat_detectTininess;
  static const uint_fast8_t rms[] = {
    softfloat_round_near_even, softfloat_round_minMag,
    softfloat_round_min, softfloat_round_max
  };
  // Cycle all four rounding modes, all 32 initial flag combinations, and both
  // tininess modes. These are caller state sentinels, not expected new flags.
  const uint_fast8_t rm = rms[supplied_blocks % 4];
  const uint_fast8_t flags = (supplied_blocks / 4) & 31;
  const uint_fast8_t tininess = (supplied_blocks / 128) & 1;
  softfloat_roundingMode = rm;
  softfloat_exceptionFlags = flags;
  softfloat_detectTininess = tininess;
  const word_t scalar_flags = fflags->val, matrix_flags = mfflags->val;
  const word_t matrix_csr = mcsr->val;
  const bool actual = mxfp_encode_supplied_block(values, e, fp4, e5m2, guarded + 1);
  if (actual != accepted) {
    fprintf(stderr, "supplied block %u: e=%u fp4=%u e5=%u accepted=%u expected=%u\n",
      supplied_blocks, e, fp4, e5m2, actual, accepted);
    abort();
  }
  const unsigned bytes = fp4 ? 16 : 32;
  for (unsigned b = 0; b < bytes; ++b) {
    const uint8_t expected = !accepted ? 0 :
      (fp4 ? codes[2 * b] | (codes[2 * b + 1] << 4) : codes[b]);
    if (guarded[b + 1] != expected) {
      fprintf(stderr, "supplied block %u: e=%u fp4=%u e5=%u byte=%u got=%02x expected=%02x\n",
        supplied_blocks, e, fp4, e5m2, b, guarded[b + 1], expected);
      abort();
    }
  }
  assert(guarded[0] == 0xa5);
  for (unsigned b = bytes + 1; b < sizeof(guarded); ++b) assert(guarded[b] == 0xa5);
  assert(memcmp(values, unchanged, sizeof(unchanged)) == 0);
  assert(softfloat_roundingMode == rm && softfloat_exceptionFlags == flags &&
    softfloat_detectTininess == tininess);
  assert(fflags->val == scalar_flags && mfflags->val == matrix_flags &&
    mcsr->val == matrix_csr);
  softfloat_roundingMode = old_rm;
  softfloat_exceptionFlags = old_flags;
  softfloat_detectTininess = old_tininess;
  ++supplied_blocks;
}

// Batch independent scalar identities into real 32-lane calls. Expected codes
// are supplied by each identity, never by a copied quantizer/production decoder.
struct supplied_batch {
  uint32_t values[32];
  uint8_t codes[32], e;
  bool fp4, e5m2;
  unsigned used;
};

static void supplied_case(struct supplied_batch *batch, uint32_t bits, unsigned code) {
  batch->values[batch->used] = bits;
  batch->codes[batch->used++] = code;
  ++supplied_lanes;
  if (batch->used == 32) {
    check_supplied_block(batch->values, batch->e, batch->fp4, batch->e5m2,
      batch->codes, true);
    batch->used = 0;
  }
}

static void supplied_signed_case(struct supplied_batch *batch, uint32_t bits,
    unsigned magnitude_code) {
  supplied_case(batch, bits, magnitude_code);
  supplied_case(batch, bits | 0x80000000u,
    magnitude_code | (batch->fp4 ? (magnitude_code ? 8 : 0) : 128));
}

static void supplied_flush(struct supplied_batch *batch) {
  while (batch->used) supplied_case(batch, 0, 0);
}

// Reuse dyadic_bits above; skip identities outside finite binary32 rather than
// accidentally wrapping its exponent construction. Every generated low-end
// value is exactly representable, including E5 e0 half-minsub (2^-144).
static bool supplied_dyadic_bits(unsigned sig, int power, uint32_t *bits) {
  if (!sig) {
    *bits = 0;
    return true;
  }
  unsigned top = 0;
  for (unsigned v = sig; v >>= 1;) ++top;
  if ((int)top + power > 127) return false;
  assert(power >= -149 && top <= 23);
  *bits = dyadic_bits(sig, power, false);
  return true;
}

static void test_supplied_encoding(void) {
  const word_t old_scalar = fflags->val, old_matrix = mfflags->val, old_csr = mcsr->val;
  fflags->val = 10; mfflags->val = 5; mcsr->val = 0xa2d;
  // Independent format dyadics, exactly as in test_formats, exercise every
  // finite positive code and every adjacent midpoint (+/- nextafter in F32).
  // Parity, not a production nearest-search, determines FP8 tie expectations.
  static const uint8_t scales[] = {0, 1, 2, 127, 251, 254};
  for (unsigned format = 0; format < 3; ++format) {
    const bool fp4 = format == 0, e5m2 = format == 2;
    const unsigned mant = fp4 ? 1 : (e5m2 ? 2 : 3);
    const int bias = fp4 ? 1 : (e5m2 ? 15 : 7);
    const unsigned last = fp4 ? 7 : (e5m2 ? 0x7b : 0x7e);
    for (unsigned si = 0; si < sizeof(scales) / sizeof(scales[0]); ++si) {
      const uint8_t e = scales[si];
      if (fp4 && e > 251) continue;
      struct supplied_batch batch = {.e = e, .fp4 = fp4, .e5m2 = e5m2};
      for (unsigned code = 0; code <= last; ++code) {
        const unsigned exp = code >> mant, fraction = code & ((1u << mant) - 1);
        const unsigned sig = (exp ? 1u << mant : 0) + fraction;
        const int power = (exp ? (int)exp : 1) - bias - (int)mant + (int)e - 127;
        uint32_t bits;
        if (supplied_dyadic_bits(sig, power, &bits)) {
          supplied_signed_case(&batch, bits, code);
        }
        if (code == last) continue;
        const unsigned next = code + 1, next_exp = next >> mant;
        const unsigned next_sig = (next_exp ? 1u << mant : 0) +
          (next & ((1u << mant) - 1));
        const int next_power = (next_exp ? (int)next_exp : 1) - bias -
          (int)mant + (int)e - 127;
        const int common_power = power < next_power ? power : next_power;
        const unsigned sum = (sig << (power - common_power)) +
          (next_sig << (next_power - common_power));
        if (supplied_dyadic_bits(sum, common_power - 1, &bits)) {
          // Positive finite bit predecessor/successor are exactly nextafter.
          supplied_signed_case(&batch, bits - 1, code);
          supplied_signed_case(&batch, bits, fp4 ? code : code + (code & 1));
          supplied_signed_case(&batch, bits + 1, next);
        }
      }
      supplied_flush(&batch);
    }
  }
  // Raw constants make sensitive subnormal/upper-envelope contracts explicit,
  // independently of both the format-dyadic loop and the production decoder.
  static const struct {
    uint32_t bits;
    uint8_t e, code, format;
  } directed[] = {
    {0x00000000, 127, 0, 0}, {0x3f400000, 127, 1, 0}, // +/-0, +/-0.75 strict
    {0x00000001, 0, 0, 0}, {0x00000001, 0, 0, 1}, {0x00000001, 0, 0, 2},
    {0x00100000, 0, 0, 0}, {0x00100001, 0, 1, 0},
    {0x00001000, 0, 0, 1}, {0x00001001, 0, 1, 1}, {0x0000f000, 0, 8, 1},
    {0x00000020, 0, 0, 2}, {0x00000021, 0, 1, 2}, {0x000000e0, 0, 4, 2},
    {0x43800000, 127, 0x78, 1}, {0x43e00000, 127, 0x7e, 1}, // 256, 448
    {0x47600000, 127, 0x7b, 2}, // 57344
    {0x7f7fffff, 254, 0x40, 1}, {0x7f7fffff, 254, 0x40, 2},
    {0x7f000000, 251, 7, 0}, // Largest FP4 e, inclusive 16d=2^127.
    {0x00000001, 254, 0, 1}, {0x00000001, 254, 0, 2}
  };
  for (unsigned i = 0; i < sizeof(directed) / sizeof(directed[0]); ++i) {
    struct supplied_batch batch = {.e = directed[i].e,
      .fp4 = directed[i].format == 0, .e5m2 = directed[i].format == 2};
    supplied_signed_case(&batch, directed[i].bits, directed[i].code);
    supplied_flush(&batch);
  }
  for (unsigned format = 0; format < 3; ++format) {
    const bool fp4 = format == 0, e5m2 = format == 2;
    uint32_t values[32];
    uint8_t codes[32];
    for (unsigned k = 0; k < 32; ++k) {
      values[k] = 0x3f800000; // Earlier successful lanes must also clear on reject.
      codes[k] = fp4 ? 2 : (e5m2 ? 0x3c : 0x38);
    }
    // Illegal e exits before SoftFloat is touched, even for otherwise valid x.
    check_supplied_block(values, 255, fp4, e5m2, NULL, false);
    if (fp4) {
      check_supplied_block(values, 252, fp4, e5m2, NULL, false);
      check_supplied_block(values, 253, fp4, e5m2, NULL, false);
      check_supplied_block(values, 254, fp4, e5m2, NULL, false);
    }
    static const uint32_t specials[] = {
      0x7fc00001, 0xffc00001, 0x7f800001, 0xff800001, 0x7f800000, 0xff800000
    };
    for (unsigned s = 0; s < sizeof(specials) / sizeof(specials[0]); ++s) {
      for (unsigned lane = 0; lane < 32; lane += 31) {
        values[lane] = specials[s];
        check_supplied_block(values, 127, fp4, e5m2, NULL, false);
        values[lane] = 0x3f800000;
      }
    }
    const uint32_t bound = fp4 ? 0x41000000 : (e5m2 ? 0x47600000 : 0x43e00000);
    for (unsigned sign = 0; sign < 2; ++sign) {
      values[31] = (bound + 1) | (sign << 31);
      check_supplied_block(values, 127, fp4, e5m2, NULL, false);
    }
    values[31] = 0x3f800000;
    // 256 blocks guarantee every rm x flags x tininess sentinel combination;
    // preserve the real architectural CSR sentinels too, not only SoftFloat.
    for (unsigned state = 0; state < 256; ++state) {
      check_supplied_block(values, 127, fp4, e5m2, codes, true);
    }
    if (fp4) {
      values[31] = 0x7f000001; // e251 envelope+1 ULP, after widening/local RM.
      check_supplied_block(values, 251, fp4, e5m2, NULL, false);
    }
  }
  fflags->val = old_scalar; mfflags->val = old_matrix; mcsr->val = old_csr;
  printf("PASS supplied-scale inverse: %u dyadic/raw lanes, %u guarded blocks, "
    "FP4 strict/FP8 RNE, rejects, scalar/matrix/SoftFloat state preserved\n",
    supplied_lanes, supplied_blocks);
}

static Decode scenario(int type, unsigned m, unsigned n, unsigned k) {
  memset(&cpu, 0, sizeof(cpu));
  memset(csr_array, 0, sizeof(csr_array));
  memset(memory, 127, sizeof(memory));
  reads = 0;
  fault_at = ~(vaddr_t)0;
  mtilem->val = m; mtilen->val = n; mtilek->val = k;
  cpu.mcfg[0].val = cpu.mcfg[1].val = type;
  cpu.mcfg[4].val = 12;
  cpu.gpr[10]._64 = 0;
  cpu.gpr[11]._64 = (k + 31) / 32 + 3; // Noncompact scale row stride.
  Decode d = {0};
  d.src1.reg = 10; d.src2.reg = 11;
  return d;
}

#ifdef NEMU_MXFP_QUEUE_TEST
static int apply_dynamic_snapshot_event(const amu_ctrl_event_t *event,
                                        const amu_ctrl_snapshot_t *scales) {
  amu_mma_snapshot_t snapshot = {0};
  snapshot.version = AMU_MMA_SNAPSHOT_VERSION;
  snapshot.struct_size = sizeof(snapshot);
  snapshot.scales = scales;
  return exec_amu_lazy_snapshot(event, &snapshot);
}
#endif

static void load_scales(Decode *d) {
  d->dest.reg = 0; mld_scale(d, false);
  d->dest.reg = 1; mld_scale(d, true);
}

static void mma(Decode *d) {
  d->dest.reg = 4; d->src1.reg = 0; d->src2.reg = 1;
  exec_mmacc(d);
  d->src1.reg = 10; d->src2.reg = 11;
}

#define EXPECT_EXCEPTION(cause, statement) do { \
  exception_cause = -1; \
  if (setjmp(exception_env) == 0) { statement; assert(!"expected exception"); } \
  assert(exception_cause == (cause)); \
} while (0)

static void assert_convert_tr_poison(void) {
  for (unsigned reg = 0; reg < 4; ++reg) {
    for (unsigned row = 0; row < ROWNUM; ++row) {
      for (unsigned b = 0; b < TRENUM8; ++b) assert(mtreg_l8(reg, row, b) == 0xa5);
    }
  }
}

#if defined(CONFIG_RV_AME_MXFP_CONVERT_LOAD) && defined(CONFIG_FPU_SOFT) && \
    !defined(CONFIG_SHARE) && !defined(CONFIG_SHARE_REF) && \
    !defined(CONFIG_SHARE_CTRL) && !defined(CONFIG_DIFFTEST_AMU_CTRL)
static unsigned convert_cases;

#define MXFP_HOST_SIGABRT 6

// Host-only fail-stop probe around the canonical EHelper. The child owns every
// state mutation, so the parent fixture remains unchanged except for creating
// the process used by the existing regression.
static void expect_convert_host_admission(Decode *d, bool b_side, bool admitted) {
  const pid_t pid = fork();
  assert(pid >= 0);
  if (pid == 0) {
    if (b_side) exec_mlbcvt_f32_mx(d);
    else exec_mlacvt_f32_mx(d);
    _exit(0);
  }
  int status = 0;
  assert(syscall(SYS_wait4, pid, &status, 0, NULL) == pid);
  const int signal = status & 0x7f;
  if (admitted) {
    assert(signal == 0 && ((status >> 8) & 0xff) == 0);
  } else {
    assert(signal == MXFP_HOST_SIGABRT);
  }
}

// Exact identities only: sig in {0,1,2,3}. Not a second encoder/search.
static uint8_t convert_exact_code(unsigned format, unsigned row, unsigned k) {
  const unsigned sig = (row + k) & 3, negative = (row + k / 2) & 1;
  static const uint8_t fp8_codes[2][4] = {{0, 0x38, 0x40, 0x44}, {0, 0x3c, 0x40, 0x42}};
  const unsigned magnitude = format ? fp8_codes[format - 1][sig] : sig;
  return magnitude | (negative ? (format ? 128 : (magnitude ? 8 : 0)) : 0);
}

static Decode prepare_convert(unsigned format, bool b_side, unsigned rows) {
  static const unsigned types[] = {0x16, 0x18, 0x17};
  Decode d = scenario(types[format], b_side ? 1 : rows, b_side ? rows : 1, 64);
  cpu.mode = MODE_M;
  d.dest.reg = b_side ? 3 : 2;
  cpu.mcfg[d.dest.reg].val = types[format];
  memset(cpu.mtr, 0xa5, sizeof(cpu.mtr));
  // Real SF writer; two different scales per row, alternating across rows.
  for (unsigned row = 0; row < rows; ++row) {
    memory[row * 2] = 126 + (row & 1);
    memory[row * 2 + 1] = 127 + (row & 1);
  }
  cpu.gpr[11]._64 = 2;
  mld_scale(&d, b_side);
  const unsigned sf = (b_side ? 4 : 0) + d.dest.reg;
  assert(cpu.mscale_mode[sf] == (format == 0 ? 10 : (format == 1 ? 7 : 8)));
  assert(cpu.mscale_generation[sf] == 1 && cpu.mscale_rows[sf] == rows &&
    cpu.mscale_blocks[sf] == 2 && cpu.mscale_valid[sf]);
  // Last source word ends at the fixture boundary, with 4B-aligned stride260.
  const unsigned extent = (rows - 1) * 260 + 256;
  cpu.gpr[10]._64 = sizeof(memory) - extent;
  cpu.gpr[11]._64 = 260;
  assert(cpu.gpr[10]._64 >= rows * 2 && !(cpu.gpr[10]._64 & 3));
  for (unsigned row = 0; row < rows; ++row) {
    for (unsigned k = 0; k < 64; ++k) {
      const uint8_t e = cpu.mscale[sf][row][k / 32];
      const uint32_t bits = dyadic_bits((row + k) & 3,
        (int)e - (format ? 127 : 128), (row + k / 2) & 1);
      const unsigned addr = cpu.gpr[10]._64 + row * 260 + k * 4;
      for (unsigned b = 0; b < 4; ++b) memory[addr + b] = bits >> (8 * b);
    }
  }
  reads = 0;
  return d;
}

static void check_convert(Decode *d, unsigned format, bool b_side,
    unsigned rows, int reject_row, int reject_block) {
  const unsigned td = d->dest.reg, bytes = format ? 64 : 32;
  const word_t old_scalar = fflags->val, old_frm = frm->val, old_fcsr = fcsr->val;
  const word_t old_flags = mfflags->val, old_csr = mcsr->val;
  const uint_fast8_t rm = softfloat_roundingMode, flags = softfloat_exceptionFlags;
  const uint_fast8_t tininess = softfloat_detectTininess;
  const size_t before_reads = reads;
  if (b_side) exec_mlbcvt_f32_mx(d);
  else exec_mlacvt_f32_mx(d);
  assert(reads - before_reads == rows * 64);
  for (unsigned reg = 0; reg < 4; ++reg) {
    for (unsigned row = 0; row < ROWNUM; ++row) {
      for (unsigned b = 0; b < TRENUM8; ++b) {
        uint8_t expected = 0xa5;
        if (reg == td && row < rows && b < bytes) {
          if ((int)row == reject_row && (int)(b / (bytes / 2)) == reject_block) {
            expected = 0;
          } else if (format) {
            expected = convert_exact_code(format, row, b);
          } else {
            expected = convert_exact_code(format, row, b * 2) |
              (convert_exact_code(format, row, b * 2 + 1) << 4);
          }
        }
        assert(mtreg_l8(reg, row, b) == expected);
      }
    }
  }
  const word_t expected_flags = old_flags | (reject_row >= 0 ? 0x10 : 0);
  assert(mfflags->val == expected_flags &&
    mcsr->val == ((old_csr & ~((word_t)31 << 3)) | (expected_flags << 3)));
  assert(fflags->val == old_scalar && frm->val == old_frm && fcsr->val == old_fcsr);
  assert(softfloat_roundingMode == rm && softfloat_exceptionFlags == flags &&
    softfloat_detectTininess == tininess);
  ++convert_cases;
}

static void test_convert_load(void) {
  static const uint_fast8_t rms[] = {softfloat_round_near_even,
    softfloat_round_minMag, softfloat_round_min, softfloat_round_max};
  const unsigned max_rows = ROWNUM < 64 ? ROWNUM : 64;
  for (unsigned format = 0; format < 3; ++format) {
    if ((format ? 64 : 32) > TRENUM8) continue;
    for (unsigned side = 0; side < 2; ++side) {
      for (unsigned shape = 0; shape < 2; ++shape) {
        const unsigned rows = shape ? max_rows : 1;
        for (unsigned rm = 0; rm < 4; ++rm) {
          Decode d = prepare_convert(format, side, rows);
          fflags->val = 19; frm->val = rm; fcsr->val = 0x6b;
          mfflags->val = rm ? 9 : 0; mcsr->val = 0xa05 | (mfflags->val << 3);
          softfloat_roundingMode = rms[rm]; softfloat_exceptionFlags = 31 - rm;
          softfloat_detectTininess = rm & 1;
          check_convert(&d, format, side, rows, -1, -1);
        }
      }
      // Reject one K32 at a time, leaving the other correctly encoded, and
      // prove q/sNaN, Inf, domain and scale rejection do not leak scalar NV.
      for (unsigned block = 0; block < 2; ++block) {
        for (unsigned kind = 0; kind < (format ? 5 : 6); ++kind) {
          Decode d = prepare_convert(format, side, 1);
          const unsigned sf = (side ? 4 : 0) + d.dest.reg;
          if (kind >= 4) {
            const word_t base = cpu.gpr[10]._64;
            memory[block] = !format && kind == 4 ? 252 : 255;
            cpu.gpr[10]._64 = 0; cpu.gpr[11]._64 = 2;
            mld_scale(&d, side);
            assert(cpu.mscale_generation[sf] == 2);
            cpu.gpr[10]._64 = base; cpu.gpr[11]._64 = 260;
          } else {
            static const uint32_t specials[] = {0x7fc00001, 0xff800001, 0x7f800000};
            const unsigned e = cpu.mscale[sf][0][block];
            const uint32_t bits = kind < 3 ? specials[kind] :
              dyadic_bits(format == 0 ? 16 : (format == 1 ? 448 : 57344),
                (int)e - (format ? 127 : 128), false) + 1;
            const unsigned addr = cpu.gpr[10]._64 + (block * 32 + 31) * 4;
            for (unsigned b = 0; b < 4; ++b) memory[addr + b] = bits >> (8 * b);
          }
          fflags->val = 0; frm->val = 3; fcsr->val = 0x60;
          mfflags->val = kind & 1 ? 16 : 8;
          mcsr->val = 0xa05 | (mfflags->val << 3);
          check_convert(&d, format, side, 1, 0, block);
        }
      }
    }
  }
  Decode d = prepare_convert(0, false, 2);
  fflags->val = 3; mfflags->val = 8; mcsr->val = 0xa45;
  // The final source word faults: no earlier prepared row may be published.
  fault_at = sizeof(memory) - 4;
  EXPECT_EXCEPTION(EX_LPF, exec_mlacvt_f32_mx(&d));
  assert(reads == 127 && mfflags->val == 8 && mcsr->val == 0xa45 && fflags->val == 3);
  assert_convert_tr_poison();
  fault_at = ~(vaddr_t)0;
  reads = 0;
  const word_t base = cpu.gpr[10]._64, stride = cpu.gpr[11]._64;
  cpu.gpr[10]._64 = base + 2;
  EXPECT_EXCEPTION(EX_LAM, exec_mlacvt_f32_mx(&d));
  assert(cpu.trapInfo.tval == base + 2);
  cpu.gpr[10]._64 = base;
  cpu.gpr[11]._64 = 259; EXPECT_EXCEPTION(EX_II, exec_mlacvt_f32_mx(&d));
  cpu.gpr[11]._64 = 252; EXPECT_EXCEPTION(EX_II, exec_mlacvt_f32_mx(&d));
  cpu.gpr[11]._64 = stride;
  d.dest.reg = 4; EXPECT_EXCEPTION(EX_II, exec_mlacvt_f32_mx(&d)); d.dest.reg = 2;
  mtilek->val = 32; EXPECT_EXCEPTION(EX_II, exec_mlacvt_f32_mx(&d)); mtilek->val = 64;
  mtilem->val = 0; EXPECT_EXCEPTION(EX_II, exec_mlacvt_f32_mx(&d));
  mtilem->val = max_rows + 1; EXPECT_EXCEPTION(EX_II, exec_mlacvt_f32_mx(&d));
  mtilem->val = 2;
  cpu.mcfg[2].val = 6; EXPECT_EXCEPTION(EX_II, exec_mlacvt_f32_mx(&d));
  cpu.mcfg[2].val = 0x19; EXPECT_EXCEPTION(EX_II, exec_mlacvt_f32_mx(&d));
  cpu.mcfg[2].val = 0x16;
  assert(!reads && mfflags->val == 8 && mcsr->val == 0xa45 && fflags->val == 3);
  assert_convert_tr_poison();
  // Faulting SF reload under a different MCFG must preserve the old published
  // mode/generation/bytes; only a later successful load may replace them.
  cpu.mcfg[2].val = 0x17;
  cpu.gpr[10]._64 = 0; cpu.gpr[11]._64 = 2;
  fault_at = 1;
  EXPECT_EXCEPTION(EX_LPF, mld_scale(&d, false));
  assert(cpu.mscale_mode[2] == 10 && cpu.mscale_generation[2] == 1 &&
    cpu.mscale_rows[2] == 2 && cpu.mscale_blocks[2] == 2 &&
    cpu.mscale[2][0][0] == 126 && cpu.mscale[2][0][1] == 127);
  fault_at = ~(vaddr_t)0;
  cpu.mcfg[2].val = 0x16;
  cpu.gpr[10]._64 = base; cpu.gpr[11]._64 = stride;
  // Metadata predicate is the actual production fail-stop admission point.
  const unsigned sf = 2;
  expect_convert_host_admission(&d, false, true);
  cpu.mcfg[2].val = 0x17;
  expect_convert_host_admission(&d, false, false);
  cpu.mcfg[2].val = 0x16;
  mtilem->val = 1;
  expect_convert_host_admission(&d, false, false);
  mtilem->val = 2;
  expect_convert_host_admission(&d, true, false); // B side has no SF.
  cpu.mscale_generation[sf] = 0;
  expect_convert_host_admission(&d, false, false);
  cpu.mscale_generation[sf] = 1;
  cpu.mscale_blocks[sf] = 1;
  expect_convert_host_admission(&d, false, false);
  cpu.mscale_blocks[sf] = 2;
  exec_minit(&d);
  for (unsigned i = 0; i < 8; ++i) assert(!cpu.mscale_mode[i]);
  expect_convert_host_admission(&d, false, false);
  printf("PASS standalone converting loads: %u A/B format/shape/rounding/reject cases, "
    "F32 stride260, active-only TR, sticky matrix NV/alias, scalar state, "
    "read-before-write, static/alignment rejects, SF/MINIT admission\n", convert_cases);
}
#else
static void test_convert_load(void) {
  Decode d = scenario(0x16, 1, 1, 64);
  memset(cpu.mtr, 0xa5, sizeof(cpu.mtr));
  fflags->val = 3; mfflags->val = 8; mcsr->val = 0xa45;
  EXPECT_EXCEPTION(EX_II, exec_mlacvt_f32_mx(&d));
  EXPECT_EXCEPTION(EX_II, exec_mlbcvt_f32_mx(&d));
  assert(!reads && mfflags->val == 8 && mcsr->val == 0xa45 && fflags->val == 3);
  assert_convert_tr_poison();
  puts("PASS converting-load feature-off/shared refusal before reads/TR/flags");
}
#endif

#if defined(CONFIG_RV_AME_MXFP_DYNAMIC_QUANT) && defined(CONFIG_FPU_SOFT) && \
    !defined(CONFIG_SHARE) && !defined(CONFIG_SHARE_REF) && \
    !defined(CONFIG_SHARE_CTRL) && !defined(CONFIG_DIFFTEST_AMU_CTRL)
static unsigned dynamic_cases, dynamic_blocks;

static void check_dynamic_scale_guard(void) {
  uint32_t values[32] = {0};
  uint8_t e = 0xa5;
  assert(mxfp_dynamic_scale_e(values, &e) && e == 0);

  const struct {
    uint32_t maximum;
    bool supported;
    uint8_t e;
  } directed[] = {
    {0x00400000, false, 0},      // E0
    {0x00800000, false, 0},      // E1
    {0x01000000, true, 0},       // E2,F0
    {0x01000400, true, 0},       // E2,F400
    {0x010003ff, false, 0},
    {0x017ffc00, true, 0},       // E2,F7ffc00
    {0x017ffc01, false, 0},
    {0x01800000, true, 1},       // E3,F0
    {0x02000000, true, 2},       // E4,F0
    {0x04000000, true, 6},       // E8,F0
    {0x40000000, true, 126},     // E128,F0
    {0x7e800000, true, 251},     // E253,F0
    {0x7f000000, false, 0},      // E254
    {0x7f7fffff, false, 0},      // E254, maximum finite
    {0x7f800000, false, 0},      // Inf
    {0x7fc00001, false, 0},      // qNaN
    {0x7f800001, false, 0},      // sNaN
  };
  for (unsigned i = 0; i < sizeof(directed) / sizeof(directed[0]); ++i) {
    memset(values, 0, sizeof(values));
    values[(i * 7) & 31] = directed[i].maximum;
    values[31] = directed[i].maximum;
    e = 0xa5;
    assert(mxfp_dynamic_scale_e(values, &e) == directed[i].supported);
    assert(e == directed[i].e);
  }
  memset(values, 0x3f, sizeof(values));
  values[31] = 0xffc00001u; // special wins even if another lane is finite.
  e = 0xa5;
  assert(!mxfp_dynamic_scale_e(values, &e) && e == 0);
  memset(values, 0, sizeof(values));
  for (unsigned lane = 0; lane < 32; ++lane) values[lane] = 0x80000000u;
  e = 0xa5;
  assert(mxfp_dynamic_scale_e(values, &e) && e == 0);
}

static uint8_t dynamic_scale_for(unsigned row, unsigned block) {
  static const uint8_t scales[8][2] = {
    {0, 1}, {2, 128}, {251, 127}, {126, 125},
    {3, 4}, {5, 64}, {127, 129}, {200, 250},
  };
  return scales[row & 7][block & 1];
}

static uint32_t dynamic_value_bits(
    unsigned row, unsigned block, unsigned lane, uint8_t e) {
  if (row == 0 && block == 0) return 0;
  static const unsigned levels[8] = {0, 1, 2, 3, 4, 6, 8, 12};
  const unsigned code = lane == 31 ? 7 : (row * 5 + block * 3 + lane) & 7;
  const bool negative = code && ((row + block + lane) & 1);
  return dyadic_bits(levels[code], (int)e - 128, negative);
}

static uint8_t dynamic_expected_code(
    unsigned row, unsigned block, unsigned lane) {
  if (row == 0 && block == 0) return 0;
  const unsigned magnitude = lane == 31 ? 7 :
    (row * 5 + block * 3 + lane) & 7;
  const bool negative = magnitude && ((row + block + lane) & 1);
  return (uint8_t)(magnitude | (negative ? 8 : 0));
}

static uint8_t dynamic_expected_scale(unsigned row, unsigned block) {
  if (row == 0 && block == 0) return 0;
  const uint8_t fixture_e = dynamic_scale_for(row, block);
  const uint32_t amax = dynamic_value_bits(row, block, 31, fixture_e);
  return (uint8_t)(((amax >> 23) & 0xff) - 2);
}

static Decode prepare_dynamic_convert(bool b_side) {
  Decode d = scenario(0x16, b_side ? 1 : 8, b_side ? 8 : 1, 64);
  cpu.mode = MODE_M;
  d.dest.reg = 2;
  cpu.mcfg[d.dest.reg].val = 0x16;
  memset(cpu.mtr, 0xa5, sizeof(cpu.mtr));
  memset(cpu.mscale, 0xa5, sizeof(cpu.mscale));
  const unsigned extent = 7 * 260 + 256;
  cpu.gpr[10]._64 = sizeof(memory) - extent;
  cpu.gpr[11]._64 = 260;
  d.isa.instr.val = (b_side ? 0xb800102bu : 0x3800102bu) |
    ((uint32_t)d.dest.reg << 7);
  for (unsigned row = 0; row < 8; ++row) {
    for (unsigned block = 0; block < 2; ++block) {
      const uint8_t e = dynamic_scale_for(row, block);
      for (unsigned lane = 0; lane < 32; ++lane) {
        const uint32_t bits = dynamic_value_bits(row, block, lane, e);
        const unsigned addr = cpu.gpr[10]._64 + row * 260 + (block * 32 + lane) * 4;
        for (unsigned b = 0; b < 4; ++b) memory[addr + b] = bits >> (8 * b);
      }
    }
  }
  reads = 0;
  return d;
}

static void check_dynamic_convert(Decode *d, bool b_side,
    int reject_row, int reject_block, uint8_t reject_scale) {
  const unsigned td = d->dest.reg;
  const unsigned sf = (b_side ? 4 : 0) + td;
  const uint32_t old_generation = cpu.mscale_generation[sf];
  const word_t old_scalar = fflags->val, old_frm = frm->val, old_fcsr = fcsr->val;
  const word_t old_flags = mfflags->val, old_csr = mcsr->val;
  const uint_fast8_t rm = softfloat_roundingMode, flags = softfloat_exceptionFlags;
  const uint_fast8_t tininess = softfloat_detectTininess;
  const size_t before_reads = reads;
  if (reject_row >= 0) {
    const uint32_t bits = reject_scale == 1 ? 0x7fc00001u :
      reject_scale == 2 ? 0x7f800000u : reject_scale == 3 ? 0x010003ffu :
      reject_scale == 4 ? 0x7f7fffffu : reject_scale == 5 ? 0x017ffc01u :
      reject_scale == 6 ? 0x00800000u : 0x7f800001u;
    for (unsigned lane = 0; lane < 32; ++lane) {
      const unsigned addr = cpu.gpr[10]._64 +
        reject_row * 260 + (reject_block * 32 + lane) * 4;
      const uint32_t lane_bits = lane == 31 ? bits : 0;
      for (unsigned b = 0; b < 4; ++b) memory[addr + b] = lane_bits >> (8 * b);
    }
  }
  if (b_side) exec_mlbcvt_f32_mx(d);
  else exec_mlacvt_f32_mx(d);
  assert(reads - before_reads == 8 * 64);

  bool success = true;
  for (unsigned reg = 0; reg < 4; ++reg) {
    for (unsigned row = 0; row < ROWNUM; ++row) {
      for (unsigned b = 0; b < TRENUM8; ++b) {
        uint8_t expected = 0xa5;
        if (reg == td && row < 8 && b < 32) {
          const unsigned block = b / 16;
          const bool reject = (int)row == reject_row && (int)block == reject_block;
          if (reject) {
            expected = 0;
          } else {
            const unsigned lane0 = (b % 16) * 2;
            expected = dynamic_expected_code(row, block, lane0) |
              (dynamic_expected_code(row, block, lane0 + 1) << 4);
          }
        }
        const uint8_t actual = mtreg_l8(reg, row, b);
        if (actual != expected) {
          fprintf(stderr, "dynamic payload mismatch side=%d rd=%u row=%u "
            "byte=%u actual=%02x expected=%02x reject=%d/%d\n",
            b_side, td, row, b, actual, expected, reject_row, reject_block);
        }
        assert(actual == expected);
      }
    }
  }
  for (unsigned row = 0; row < ROWNUM; ++row) {
    for (unsigned block = 0; block < AME_MAX_SCALE_BLOCKS; ++block) {
      uint8_t expected = 127;
      if (row < 8 && block < 2) {
        const bool reject = (int)row == reject_row && (int)block == reject_block;
        expected = reject ? 0 : dynamic_expected_scale(row, block);
        success &= !reject;
      }
      assert(cpu.mscale[sf][row][block] == expected);
    }
  }
  assert(cpu.mscale_valid[sf] == success);
  assert(cpu.mscale_rows[sf] == 8 && cpu.mscale_blocks[sf] == 2);
  assert(cpu.mscale_generation[sf] == old_generation + 1);
  assert(cpu.mscale_source_stride[sf] == 260 && cpu.mscale_mode[sf] == 10);
  const word_t expected_flags = old_flags | (success ? 0 : 0x10);
  assert(mfflags->val == expected_flags &&
    mcsr->val == ((old_csr & ~((word_t)31 << 3)) | (expected_flags << 3)));
  assert(fflags->val == old_scalar && frm->val == old_frm && fcsr->val == old_fcsr);
  assert(softfloat_roundingMode == rm && softfloat_exceptionFlags == flags &&
    softfloat_detectTininess == tininess);
  ++dynamic_cases;
  dynamic_blocks += 16;
}

static void test_dynamic_convert_load(void) {
  // Exact instruction identities and literal tr0..3 rd field.
  assert((0x3800102bu & 0xfe007e7fu) == 0x3800102bu);
  assert((0xb800102bu & 0xfe007e7fu) == 0xb800102bu);
  assert(((0x3800102bu | (3u << 7)) & 0xfe007e7fu) ==
    (0x3800102bu & 0xfe007e7fu));
  assert(((0x3800102bu | (4u << 7)) & 0xfe007e7fu) !=
    (0x3800102bu & 0xfe007e7fu));
  check_dynamic_scale_guard();

  static const uint_fast8_t rms[] = {softfloat_round_near_even,
    softfloat_round_minMag, softfloat_round_min, softfloat_round_max};
  // A supplied load remains a pre-existing SF reader. MAX generation makes
  // any dynamic-style increment illegal, so successful readback proves that
  // the overflow guard is scoped only to generated-SF dynamic attempts.
  {
    Decode supplied = prepare_convert(0, false, 1);
    const unsigned sf = 2;
    cpu.mscale_generation[sf] = UINT32_MAX;
    reads = 0;
    exec_mlacvt_f32_mx(&supplied);
    assert(reads == 64 && cpu.mscale_generation[sf] == UINT32_MAX);
    assert(cpu.mscale_valid[sf] && cpu.mscale_rows[sf] == 1 &&
      cpu.mscale_blocks[sf] == 2);
  }
  for (unsigned side = 0; side < 2; ++side) {
    for (unsigned rm = 0; rm < 4; ++rm) {
      Decode d = prepare_dynamic_convert(side);
      cpu.mscale_generation[(side ? 4 : 0) + d.dest.reg] = rm;
      fflags->val = 19; frm->val = rm; fcsr->val = 0x6b;
      mfflags->val = rm ? 9 : 0; mcsr->val = 0xa05 | (mfflags->val << 3);
      softfloat_roundingMode = rms[rm]; softfloat_exceptionFlags = 31 - rm;
      softfloat_detectTininess = rm & 1;
      check_dynamic_convert(&d, side, -1, -1, 0);
    }
    for (unsigned block = 0; block < 2; ++block) {
      for (unsigned kind = 1; kind <= 7; ++kind) {
        Decode d = prepare_dynamic_convert(side);
        fflags->val = 3; frm->val = 4; fcsr->val = 0x80;
        mfflags->val = kind & 1 ? 8 : 16;
        mcsr->val = 0xa05 | (mfflags->val << 3);
        check_dynamic_convert(&d, side, 3, block, kind);
      }
    }
  }

  Decode d = prepare_dynamic_convert(false);
  fault_at = cpu.gpr[10]._64 + 7 * 260 + 63 * 4;
  fflags->val = 7; mfflags->val = 8; mcsr->val = 0xa45;
  EXPECT_EXCEPTION(EX_LPF, exec_mlacvt_f32_mx(&d));
  assert(reads == 511 && mfflags->val == 8 && mcsr->val == 0xa45 && fflags->val == 7);
  assert_convert_tr_poison();
  fault_at = ~(vaddr_t)0;
  reads = 0;

  const word_t base = cpu.gpr[10]._64, stride = cpu.gpr[11]._64;
  d.dest.reg = 4; EXPECT_EXCEPTION(EX_II, exec_mlacvt_f32_mx(&d)); d.dest.reg = 2;
  cpu.gpr[10]._64 = base + 2; EXPECT_EXCEPTION(EX_LAM, exec_mlacvt_f32_mx(&d));
  cpu.gpr[10]._64 = base;
  cpu.gpr[11]._64 = 252; EXPECT_EXCEPTION(EX_II, exec_mlacvt_f32_mx(&d));
  cpu.gpr[11]._64 = stride;
  mtilek->val = 32; EXPECT_EXCEPTION(EX_II, exec_mlacvt_f32_mx(&d)); mtilek->val = 64;
  mtilem->val = 7; EXPECT_EXCEPTION(EX_II, exec_mlacvt_f32_mx(&d));
  mtilem->val = 9; EXPECT_EXCEPTION(EX_II, exec_mlacvt_f32_mx(&d));
  mtilem->val = 8;
  cpu.mcfg[2].val = 0x17; EXPECT_EXCEPTION(EX_II, exec_mlacvt_f32_mx(&d));
  cpu.mcfg[2].val = 0x16;
  assert(!reads && cpu.mscale_valid[2] == 0 && cpu.mscale_generation[2] == 0);
  assert_convert_tr_poison();
  cpu.mscale_generation[2] = UINT32_MAX;
  EXPECT_EXCEPTION(EX_II, exec_mlacvt_f32_mx(&d));
  assert(cpu.mscale_generation[2] == UINT32_MAX && !reads);
  cpu.mscale_generation[2] = 0;
  printf("PASS standalone dynamic MXFP4 converting loads: %u A/B cases, "
    "%u K32 blocks, guarded E/F admission, strict-lower payload, generated "
    "SF generation/valid/padding, reject NV, read-before-write, scalar state\n",
    dynamic_cases, dynamic_blocks);
}
#elif defined(CONFIG_RV_AME_MXFP_CONVERT_LOAD) && \
      defined(CONFIG_RV_AME_MXFP_DYNAMIC_QUANT) && defined(CONFIG_FPU_SOFT) && \
      defined(CONFIG_SHARE_REF) && defined(CONFIG_DIFFTEST_AMU_CTRL) && \
      !defined(CONFIG_SHARE_CTRL)
static uint32_t shared_dynamic_bits(unsigned row, unsigned block,
                                    unsigned lane, uint8_t e) {
  static const unsigned levels[8] = {0, 1, 2, 3, 4, 6, 8, 12};
  const unsigned code = lane == 31 ? 7 : (row * 5 + block * 3 + lane) & 7;
  const bool negative = code && ((row + block + lane) & 1);
  return dyadic_bits(levels[code], (int)e - 128, negative);
}

static Decode prepare_shared_dynamic(bool b_side, bool reject) {
  Decode d = scenario(0x16, b_side ? 1 : 8, b_side ? 8 : 1, 64);
  cpu.mode = MODE_M;
  d.pc = b_side ? 0x80002400 : 0x80002000;
  d.dest.reg = 2;
  cpu.mcfg[d.dest.reg].val = 0x16;
  cpu.mcfg[1].val = 0x16;
  memset(cpu.mtr, 0xa5, sizeof(cpu.mtr));
  memset(cpu.mscale, 0xa5, sizeof(cpu.mscale));
  cpu.gpr[10]._64 = 1024;
  cpu.gpr[11]._64 = 260;
  d.isa.instr.val = (b_side ? 0xb800102bu : 0x3800102bu) |
    ((uint32_t)d.dest.reg << 7);
  for (unsigned row = 0; row < 8; ++row) {
    for (unsigned block = 0; block < 2; ++block) {
      const uint8_t e = (uint8_t)((row * 17 + block * 7) % 252);
      for (unsigned lane = 0; lane < 32; ++lane) {
        const uint32_t bits = shared_dynamic_bits(row, block, lane, e);
        const unsigned addr = (unsigned)cpu.gpr[10]._64 +
          row * 260 + (block * 32 + lane) * 4;
        for (unsigned b = 0; b < 4; ++b) memory[addr + b] = bits >> (8 * b);
      }
    }
  }
  if (reject) {
    const unsigned addr = (unsigned)cpu.gpr[10]._64 + 3 * 260 + 32 * 4 + 31 * 4;
    for (unsigned b = 0; b < 4; ++b) memory[addr + b] = 0x7f800000u >> (8 * b);
  }
  reads = 0;
  return d;
}

static void expect_shared_raw_exec_failstop(const amu_ctrl_event_t *event) {
  const pid_t pid = fork();
  assert(pid >= 0);
  if (pid == 0) {
    uint8_t unused[TLEN / 8] = {0};
    exec_amu((void *)event, unused);
    _exit(0);
  }
  int status = 0;
  assert(syscall(SYS_wait4, pid, &status, 0, NULL) == pid);
  assert((status & 0x7f) == 6);
}

static void check_shared_dynamic_once(bool b_side, bool reject) {
  amu_ctrl_queue_reset();
  assert(amu_snapshot_negotiate(AMU_CTRL_SNAPSHOT_VERSION,
    sizeof(amu_ctrl_snapshot_t), sizeof(amu_mma_snapshot_t),
    sizeof(amu_ctrl_event_t), TLEN / 8, ALEN / 8) == 0);
  Decode d = prepare_shared_dynamic(b_side, reject);
  const uint64_t source_base = cpu.gpr[10]._64;
  const uint64_t source_stride = cpu.gpr[11]._64;
  prev_s = &d;
  const unsigned side = b_side ? 1 : 0;
  const unsigned sf = side * 4 + d.dest.reg;
  const uint32_t old_generation = cpu.mscale_generation[sf];
  const word_t old_matrix_flags = 8, old_mcsr = 0xa45;
  mfflags->val = old_matrix_flags;
  mcsr->val = old_mcsr;
  const uint64_t old_epoch = cpu.mscale_epoch;
  if (b_side) exec_mlbcvt_f32_mx(&d);
  else exec_mlacvt_f32_mx(&d);
  assert(reads == 8 * 64);
  assert(cpu.mscale_generation[sf] == old_generation + 1);
  assert(cpu.mscale_rows[sf] == 8 && cpu.mscale_blocks[sf] == 2);
  assert(cpu.mscale_mode[sf] == 10 && cpu.mscale_source_stride[sf] == 260);
  assert(cpu.mscale_valid[sf] == !reject);
  assert(mfflags->val == (old_matrix_flags | (reject ? 0x10 : 0)));
  assert(mcsr->val == ((old_mcsr & ~((word_t)31 << 3)) | (mfflags->val << 3)));
  assert(cpu.mscale_epoch == old_epoch && amu_ctrl_queue_size() == 1);
  amu_ctrl_event_t ctrl = amu_ctrl_queue_front();
  assert(ctrl.pc == d.pc && ctrl.md == d.dest.reg &&
    ctrl.types2 == !b_side && ctrl.isConvertF32 && ctrl.isDynamicMxfpQuant);
  // AMUCtrlBuffer's MLS DifftestModule is dontCare=true and drives none of
  // these fields. The typed path must not require accidental zeroes.
  ctrl.rm = 0xa5;
  ctrl.ms1 = 0x5a;
  ctrl.ms2 = 0x3c;
  ctrl.mtilek = 0xc35a;
  expect_shared_raw_exec_failstop(&ctrl);
  for (unsigned reg = 0; reg < 4; ++reg) {
    for (unsigned row = 0; row < ROWNUM; ++row) {
      for (unsigned byte = 0; byte < TRENUM8; ++byte)
        assert(cpu.mtr[reg][row]._8[byte] == 0xa5);
    }
  }
  exec_minit(&d);
  assert(cpu.mscale_epoch == old_epoch + 1 && !cpu.mscale_generation[sf]);
  // The producer fully consumed the 512 F32 words before enqueue. Mutating
  // every source word now must not affect the retained TR observation.
  for (unsigned row = 0; row < 8; ++row) {
    for (unsigned k = 0; k < 64; ++k) {
      const uint64_t addr = source_base + row * source_stride + k * 4;
      for (unsigned b = 0; b < 4; ++b) memory[addr + b] ^= 0xff;
    }
  }

  uint8_t sf_bytes[2][64], expected_tr[256];
  memset(sf_bytes, 0xa5, sizeof(sf_bytes));
  amu_ctrl_snapshot_t snapshot = {0};
  snapshot.version = AMU_CTRL_SNAPSHOT_VERSION;
  snapshot.struct_size = sizeof(snapshot);
  snapshot.expected_tr = (amu_matrix_view_t){expected_tr, sizeof(expected_tr), 32, 0};
  for (unsigned s = 0; s < 2; ++s) {
    snapshot.scales[s].bytes = sf_bytes[s];
    snapshot.scales[s].capacity = sizeof(sf_bytes[s]);
  }
  amu_ctrl_snapshot_t bad = snapshot;
  bad.expected_tr.capacity = 255;
  amu_ctrl_event_t compared = ctrl;
  assert(check_amu_ctrl_snapshot(&compared, &bad) == -2);
  assert(amu_ctrl_queue_size() == 1 && cpu.mtr[ctrl.md][0]._8[0] == 0xa5);
  assert(check_amu_ctrl_snapshot(&compared, &snapshot) == 0);
  assert(amu_ctrl_queue_empty() && cpu.mscale_epoch == old_epoch + 1);
  assert(snapshot.epoch == old_epoch && snapshot.raised_flags == (reject ? 0x10u : 0u));
  const amu_scale_metadata_t *meta = &snapshot.scales[side].metadata;
  assert(meta->valid == !reject && meta->generation == old_generation + 1 &&
    meta->rows == 8 && meta->blocks == 2 && meta->row_pitch == 2 &&
    meta->reg_id == ctrl.md && meta->side == side && meta->source_stride == 260);
  assert(snapshot.scales[!side].metadata.rows == 0);

  uint8_t bad_tr[256], bad_sf[2][64];
  memcpy(bad_tr, expected_tr, sizeof(bad_tr));
  memcpy(bad_sf, sf_bytes, sizeof(bad_sf));
  const word_t flags_before_apply = mfflags->val;
  const word_t mcsr_before_apply = mcsr->val;
  for (unsigned fault = 0; fault < 16; ++fault) {
    amu_ctrl_snapshot_t bad = snapshot;
    memcpy(bad_tr, expected_tr, sizeof(bad_tr));
    memcpy(bad_sf, sf_bytes, sizeof(bad_sf));
    bad.expected_tr.bytes = bad_tr;
    bad.scales[0].bytes = bad_sf[0];
    bad.scales[1].bytes = bad_sf[1];
    if (fault == 0) ++bad.epoch;
    if (fault == 1) bad.scales[side].metadata.side ^= 1;
    if (fault == 2) ++bad.scales[side].metadata.generation;
    if (fault == 3) bad.scales[side].metadata.valid ^= 1;
    if (fault == 4) bad.raised_flags ^= 0x10;
    if (fault == 5) bad_tr[0] ^= 1;
    if (fault == 6) bad_sf[side][0] ^= 1;
    if (fault == 7) bad.scales[side].capacity = 15;
    if (fault == 8) bad.expected_tr.capacity = 255;
    if (fault == 9) bad.expected_tr.row_pitch = 31;
    if (fault == 10) bad.version = 1;
    if (fault == 11) continue; // Outer-matrix refusal is tested below.
    if (fault == 12) ++bad.scales[side].metadata.blocks;
    if (fault == 13) ++bad.scales[side].metadata.source_stride;
    if (fault == 14) bad.scales[side].metadata.reg_id ^= 1;
    if (fault == 15) ++bad.scales[side].metadata.rows;
    assert(apply_dynamic_snapshot_event(&ctrl, &bad) != 0);
    assert(mfflags->val == flags_before_apply && mcsr->val == mcsr_before_apply);
    for (unsigned reg = 0; reg < 4; ++reg) {
      for (unsigned row = 0; row < ROWNUM; ++row) {
        for (unsigned byte = 0; byte < TRENUM8; ++byte)
          assert(cpu.mtr[reg][row]._8[byte] == 0xa5);
      }
    }
  }

  const size_t apply_reads = reads;
  amu_ctrl_event_t wrong = ctrl;
  wrong.pc ^= 4;
  assert(apply_dynamic_snapshot_event(&wrong, &snapshot) != 0);
  assert(cpu.mtr[ctrl.md][0]._8[0] == 0xa5);
  amu_mma_snapshot_t outer = {0};
  outer.version = AMU_MMA_SNAPSHOT_VERSION;
  outer.struct_size = sizeof(outer);
  outer.scales = &snapshot;
  outer.raised_flags = 0xa5;
  amu_mma_snapshot_t bad_outer = outer;
  bad_outer.matrices[0].capacity = 1;
  assert(exec_amu_lazy_snapshot(&ctrl, &bad_outer) == -2);
  assert(bad_outer.raised_flags == 0xa5);
  assert(mfflags->val == flags_before_apply && mcsr->val == mcsr_before_apply);
  for (unsigned reg = 0; reg < 4; ++reg) {
    for (unsigned row = 0; row < ROWNUM; ++row) {
      for (unsigned byte = 0; byte < TRENUM8; ++byte)
        assert(cpu.mtr[reg][row]._8[byte] == 0xa5);
    }
  }
  assert(exec_amu_lazy_snapshot(&ctrl, &outer) == 0);
  assert(outer.raised_flags == (reject ? 0x10u : 0u));
  assert(reads == apply_reads);
  for (unsigned row = 0; row < 8; ++row) {
    assert(memcmp(cpu.mtr[ctrl.md][row]._8, expected_tr + row * 32, 32) == 0);
    assert(cpu.mtr[ctrl.md][row]._8[32] == 0xa5);
  }
  assert(mfflags->val == (old_matrix_flags | (reject ? 0x10 : 0)));
  assert(apply_dynamic_snapshot_event(&ctrl, &snapshot) != 0);
}

static void apply_pending_shared_dynamic(unsigned side, uint64_t epoch,
                                         uint32_t generation, bool reject,
                                         uint8_t expected_tr[256]) {
  amu_ctrl_event_t ctrl = amu_ctrl_queue_front();
  assert(ctrl.isDynamicMxfpQuant && ctrl.types2 == (side == 0));
  uint8_t sf_bytes[2][64];
  amu_ctrl_snapshot_t snapshot = {0};
  snapshot.version = AMU_CTRL_SNAPSHOT_VERSION;
  snapshot.struct_size = sizeof(snapshot);
  snapshot.expected_tr = (amu_matrix_view_t){expected_tr, 256, 32, 0};
  for (unsigned s = 0; s < 2; ++s) {
    snapshot.scales[s].bytes = sf_bytes[s];
    snapshot.scales[s].capacity = sizeof(sf_bytes[s]);
  }
  amu_ctrl_event_t compared = ctrl;
  assert(check_amu_ctrl_snapshot(&compared, &snapshot) == 0);
  const amu_scale_metadata_t *meta = &snapshot.scales[side].metadata;
  assert(snapshot.epoch == epoch && snapshot.raised_flags == (reject ? 16u : 0u));
  assert(meta->valid == !reject && meta->generation == generation &&
    meta->rows == 8 && meta->blocks == 2 && meta->row_pitch == 2 &&
    meta->reg_id == ctrl.md && meta->side == side && meta->source_stride == 260);

  const size_t apply_reads = reads;
  assert(apply_dynamic_snapshot_event(&ctrl, &snapshot) == 0);
  assert(reads == apply_reads);
}

static void test_shared_dynamic_order(void) {
  assert(amu_snapshot_negotiate(AMU_CTRL_SNAPSHOT_VERSION,
    sizeof(amu_ctrl_snapshot_t), sizeof(amu_mma_snapshot_t),
    sizeof(amu_ctrl_event_t), TLEN / 8, ALEN / 8) == 0);

  // Older MMA remains ahead of the dynamic result and reads the pre-existing
  // global TR, not the younger producer's local compact bytes.
  amu_ctrl_queue_reset();
  Decode older = {0};
  older.pc = 0x80003000;
  prev_s = &older;
  amu_ctrl_queue_mma_emplace(4, 0, false, true, 2, 1, 1, 1, 64, 7, 7, 2, 0);
  Decode dynamic_a = prepare_shared_dynamic(false, false);
  prev_s = &dynamic_a;
  exec_mlacvt_f32_mx(&dynamic_a);
  assert(amu_ctrl_queue_size() == 2);
  amu_ctrl_event_t old_mma = amu_ctrl_queue_front();
  assert(old_mma.op == 0 && old_mma.pc == older.pc);
  for (unsigned reg = 0; reg < 4; ++reg) {
    for (unsigned row = 0; row < ROWNUM; ++row) {
      for (unsigned byte = 0; byte < TRENUM8; ++byte)
        assert(cpu.mtr[reg][row]._8[byte] == 0xa5);
    }
  }
  amu_ctrl_event_t matched = old_mma;
  assert(check_amu_ctrl(&matched) == 0);
  uint8_t *mma_res = calloc(1, ALEN / 8);
  uint8_t *mma_a = malloc(TLEN / 8), *mma_b = malloc(TLEN / 8);
  uint8_t *mma_c = malloc(ALEN / 8);
  assert(mma_res && mma_a && mma_b && mma_c);
  exec_amu_lazy(&old_mma, mma_res, mma_a, mma_b, mma_c);
  for (unsigned i = 0; i < TLEN / 8; ++i) {
    assert(mma_a[i] == 0xa5 && mma_b[i] == 0xa5);
  }
  free(mma_res); free(mma_a); free(mma_b); free(mma_c);
  uint8_t expected_mma[256] = {0};
  const uint64_t dynamic_epoch = cpu.mscale_epoch;
  const uint32_t dynamic_generation = cpu.mscale_generation[2];
  apply_pending_shared_dynamic(0, dynamic_epoch,
    dynamic_generation, false, expected_mma);

  // An older ordinary packed MLS may execute after the younger dynamic task
  // has been enqueued but before that dynamic result reaches SWROB apply. Its
  // physical write lands first; the later dynamic install is final.
  amu_ctrl_queue_reset();
  older = (Decode){0};
  older.pc = 0x80003400;
  prev_s = &older;
  amu_ctrl_queue_mls_emplace(2, false, false, false, true, 0, 64, 1, 32, 7);
  Decode dynamic_b = prepare_shared_dynamic(false, false);
  prev_s = &dynamic_b;
  exec_mlacvt_f32_mx(&dynamic_b);
  // Provide a real B-side scale owner before the manual younger MMA is
  // enqueued, so OwnedAmuCtrl captures both participants rather than relying
  // on later live metadata.
  memset(cpu.mscale[5], 127, sizeof(cpu.mscale[5]));
  cpu.mscale_valid[5] = 1;
  cpu.mscale_rows[5] = 1;
  cpu.mscale_blocks[5] = 2;
  cpu.mscale_generation[5] = 1;
  cpu.mscale_source_stride[5] = 2;
  cpu.mscale_mode[5] = 10;
  memset(cpu.mtr[1][0]._8, 0x22, 32);
  Decode younger_mma = {0};
  younger_mma.pc = 0x80003600;
  prev_s = &younger_mma;
  amu_ctrl_queue_mma_emplace(4, 0, false, true, 2, 1, 8, 1, 64, 7, 7, 2, 10);
  amu_ctrl_event_t ordinary = amu_ctrl_queue_front();
  assert(ordinary.op == 1 && !ordinary.isConvertF32 && ordinary.pc == older.pc);
  matched = ordinary;
  assert(check_amu_ctrl(&matched) == 0);
  for (unsigned byte = 0; byte < 32; ++byte) memory[byte] = (uint8_t)(0x30 + byte);
  uint8_t observed[TLEN / 8];
  memset(observed, 0xa5, sizeof(observed));
  for (unsigned byte = 0; byte < 32; ++byte) observed[byte] = memory[byte];
  allow_matrix_memory = true;
  assert(exec_amu(&ordinary, observed) == 0);
  allow_matrix_memory = false;
  assert(cpu.mtr[2][0]._8[0] == 0x30);
  uint8_t expected_ordinary[256] = {0};
  apply_pending_shared_dynamic(0, cpu.mscale_epoch,
    cpu.mscale_generation[2], false, expected_ordinary);
  for (unsigned row = 0; row < 8; ++row) {
    assert(memcmp(cpu.mtr[2][row]._8, expected_ordinary + row * 32, 32) == 0);
  }
  amu_ctrl_event_t younger = amu_ctrl_queue_front();
  assert(younger.op == 0 && younger.pc == younger_mma.pc &&
    younger.computeType == 10 && younger.types1 == 7 && younger.types2 == 7 &&
    younger.typed == 2 && younger.mtilem == 8 && younger.mtilen == 1 &&
    younger.mtilek == 64);

  uint8_t scale_bytes[2][64];
  memset(scale_bytes, 0xa5, sizeof(scale_bytes));
  amu_ctrl_snapshot_t scales = {0};
  scales.version = AMU_CTRL_SNAPSHOT_VERSION;
  scales.struct_size = sizeof(scales);
  for (unsigned side = 0; side < 2; ++side) {
    scales.scales[side].bytes = scale_bytes[side];
    scales.scales[side].capacity = sizeof(scale_bytes[side]);
  }
  assert(check_amu_ctrl_snapshot(&younger, &scales) == 0);
  const amu_scale_metadata_t *scale_a = &scales.scales[0].metadata;
  const amu_scale_metadata_t *scale_b = &scales.scales[1].metadata;
  assert(scale_a->valid && scale_a->generation == dynamic_generation &&
    scale_a->rows == 8 && scale_a->blocks == 2 && scale_a->row_pitch == 2 &&
    scale_a->reg_id == 2 && scale_a->side == 0 && scale_a->source_stride == 260);
  assert(scale_b->valid && scale_b->generation == 1 && scale_b->rows == 1 &&
    scale_b->blocks == 2 && scale_b->row_pitch == 2 && scale_b->reg_id == 1 &&
    scale_b->side == 1 && scale_b->source_stride == 2);
  for (unsigned row = 0; row < 8; ++row) {
    assert(memcmp(scale_bytes[0] + row * 2, cpu.mscale[2][row], 2) == 0);
  }
  assert(scale_bytes[0][16] == 0xa5);
  assert(scale_bytes[1][0] == 127 && scale_bytes[1][1] == 127 &&
    scale_bytes[1][2] == 0xa5);

  enum { YOUNG_AB_PITCH = 40, YOUNG_C_PITCH = 8, YOUNG_VIEW_CAP = 8 * 40 + 8 };
  uint8_t young_a[YOUNG_VIEW_CAP], young_b[YOUNG_VIEW_CAP];
  uint8_t young_c[8 * YOUNG_C_PITCH + 8], young_out[8 * YOUNG_C_PITCH + 8];
  memset(young_a, 0xa5, sizeof(young_a));
  memset(young_b, 0xa5, sizeof(young_b));
  memset(young_c, 0, sizeof(young_c));
  memset(young_out, 0xa5, sizeof(young_out));
  amu_mma_snapshot_t young_lazy = {0};
  young_lazy.version = AMU_MMA_SNAPSHOT_VERSION;
  young_lazy.struct_size = sizeof(young_lazy);
  young_lazy.scales = &scales;
  young_lazy.matrices[0] = (amu_matrix_view_t){young_a, sizeof(young_a), YOUNG_AB_PITCH, 0};
  young_lazy.matrices[1] = (amu_matrix_view_t){young_b, sizeof(young_b), YOUNG_AB_PITCH, 0};
  young_lazy.matrices[2] = (amu_matrix_view_t){young_c, sizeof(young_c), YOUNG_C_PITCH, 0};
  young_lazy.matrices[3] = (amu_matrix_view_t){young_out, sizeof(young_out), YOUNG_C_PITCH, 0};
  assert(exec_amu_lazy_snapshot(&younger, &young_lazy) == 0);
  assert(young_lazy.raised_flags <= 31);
  for (unsigned row = 0; row < 8; ++row) {
    assert(memcmp(young_a + row * YOUNG_AB_PITCH,
      expected_ordinary + row * 32, 32) == 0);
    for (unsigned byte = 32; byte < YOUNG_AB_PITCH; ++byte)
      assert(young_a[row * YOUNG_AB_PITCH + byte] == 0xa5);
    uint32_t result;
    memcpy(&result, young_out + row * YOUNG_C_PITCH, sizeof(result));
    assert(result == macc_l32(4, row, 0));
    for (unsigned byte = 4; byte < YOUNG_C_PITCH; ++byte)
      assert(young_out[row * YOUNG_C_PITCH + byte] == 0xa5);
  }
  for (unsigned byte = 0; byte < 32; ++byte) assert(young_b[byte] == 0x22);
  for (unsigned byte = 32; byte < YOUNG_AB_PITCH; ++byte) assert(young_b[byte] == 0xa5);
  for (unsigned byte = 0; byte < 8; ++byte) assert(young_c[byte] == 0);
  for (unsigned byte = 4; byte < YOUNG_C_PITCH; ++byte) assert(young_c[byte] == 0);
}

static void test_shared_dynamic_preserves_young_sf(void) {
  amu_ctrl_queue_reset();
  assert(amu_snapshot_negotiate(AMU_CTRL_SNAPSHOT_VERSION,
    sizeof(amu_ctrl_snapshot_t), sizeof(amu_mma_snapshot_t),
    sizeof(amu_ctrl_event_t), TLEN / 8, ALEN / 8) == 0);
  Decode d = prepare_shared_dynamic(false, false);
  prev_s = &d;
  exec_mlacvt_f32_mx(&d);
  amu_ctrl_event_t old = amu_ctrl_queue_front();
  uint8_t old_sf[2][64], expected_tr[256];
  amu_ctrl_snapshot_t snapshot = {0};
  snapshot.version = AMU_CTRL_SNAPSHOT_VERSION;
  snapshot.struct_size = sizeof(snapshot);
  snapshot.expected_tr = (amu_matrix_view_t){expected_tr, 256, 32, 0};
  for (unsigned side = 0; side < 2; ++side) {
    snapshot.scales[side].bytes = old_sf[side];
    snapshot.scales[side].capacity = sizeof(old_sf[side]);
  }
  amu_ctrl_event_t compared = old;
  assert(check_amu_ctrl_snapshot(&compared, &snapshot) == 0);

  // Advance the same live slot after the old result was delivered, then clear
  // the whole bank and establish a distinct younger publication.
  cpu.gpr[10]._64 = 4096;
  cpu.gpr[11]._64 = 2;
  for (unsigned row = 0; row < 8; ++row)
    for (unsigned block = 0; block < 2; ++block)
      memory[4096 + row * 2 + block] = (uint8_t)(3 + row + block);
  mld_scale(&d, false);
  const uint32_t advanced_generation = cpu.mscale_generation[2];
  exec_minit(&d);
  assert(!cpu.mscale_generation[2]);
  for (unsigned row = 0; row < 8; ++row)
    for (unsigned block = 0; block < 2; ++block)
      memory[4096 + row * 2 + block] = (uint8_t)(31 + row + block);
  mld_scale(&d, false);

  uint8_t young_bytes[ROWNUM][AME_MAX_SCALE_BLOCKS];
  memcpy(young_bytes, cpu.mscale[2], sizeof(young_bytes));
  const uint8_t young_valid = cpu.mscale_valid[2];
  const uint16_t young_rows = cpu.mscale_rows[2];
  const uint16_t young_blocks = cpu.mscale_blocks[2];
  const uint32_t young_generation = cpu.mscale_generation[2];
  const uint64_t young_stride = cpu.mscale_source_stride[2];
  const uint8_t young_mode = cpu.mscale_mode[2];
  const uint64_t young_epoch = cpu.mscale_epoch;
  const word_t young_flags = mfflags->val;
  assert(young_generation && young_generation != advanced_generation);

  assert(apply_dynamic_snapshot_event(&old, &snapshot) == 0);
  assert(memcmp(cpu.mscale[2], young_bytes, sizeof(young_bytes)) == 0);
  assert(cpu.mscale_valid[2] == young_valid &&
    cpu.mscale_rows[2] == young_rows && cpu.mscale_blocks[2] == young_blocks &&
    cpu.mscale_generation[2] == young_generation &&
    cpu.mscale_source_stride[2] == young_stride &&
    cpu.mscale_mode[2] == young_mode && cpu.mscale_epoch == young_epoch &&
    mfflags->val == young_flags);

  cpu.mtr[old.md][0]._8[0] = 0xee;
  assert(apply_dynamic_snapshot_event(&old, &snapshot) != 0);
  assert(cpu.mtr[old.md][0]._8[0] == 0xee);
}

static void test_shared_dynamic_no_skip_same_pc(void) {
  amu_ctrl_queue_reset();
  assert(amu_snapshot_negotiate(AMU_CTRL_SNAPSHOT_VERSION,
    sizeof(amu_ctrl_snapshot_t), sizeof(amu_mma_snapshot_t),
    sizeof(amu_ctrl_event_t), TLEN / 8, ALEN / 8) == 0);
  amu_ctrl_event_t events[2] = {0}, compared;
  uint8_t refs[2][256], trs[2][257], sfs[2][2][17];
  amu_ctrl_snapshot_t snapshots[2] = {{0}};
  for (unsigned i = 0; i < 2; ++i) {
    events[i].valid = 1;
    events[i].op = 1;
    events[i].md = 2;
    events[i].types2 = 1;
    events[i].typed = 2;
    events[i].base = 256;
    events[i].stride = 256;
    events[i].mtilem = 8;
    events[i].mtilen = 64;
    events[i].pc = 0x80003c00;
    events[i].computeType = 10;
    events[i].isConvertF32 = 1;
    events[i].isDynamicMxfpQuant = 1;
    events[i].rm = (uint8_t)(0xa0 + i);
    events[i].ms1 = (uint8_t)(0x30 + i);
    events[i].ms2 = (uint8_t)(0x50 + i);
    events[i].mtilek = (uint16_t)(0xc000 + i);
    for (unsigned byte = 0; byte < 256; ++byte)
      refs[i][byte] = (uint8_t)(0x40 + i * 16 + (byte & 15));
    memset(cpu.mscale[2], (int)(5 + i), sizeof(cpu.mscale[2]));
    cpu.mscale_valid[2] = 1;
    cpu.mscale_rows[2] = 8;
    cpu.mscale_blocks[2] = 2;
    cpu.mscale_generation[2] = 1 + i;
    cpu.mscale_source_stride[2] = events[i].stride;
    cpu.mscale_mode[2] = 10;
    amu_matrix_view_t ref = {refs[i], 256, 32, 0};
    amu_ctrl_queue_push(events[i], 0, &ref);
  }

  for (unsigned i = 0; i < 2; ++i) {
    amu_ctrl_snapshot_t *snapshot = &snapshots[i];
    snapshot->version = AMU_CTRL_SNAPSHOT_VERSION;
    snapshot->struct_size = sizeof(*snapshot);
    snapshot->expected_tr = (amu_matrix_view_t){trs[i], 256, 32, 0};
    for (unsigned side = 0; side < 2; ++side) {
      snapshot->scales[side].bytes = sfs[i][side];
      snapshot->scales[side].capacity = 16;
    }
    compared = events[i];
    assert(check_amu_ctrl_snapshot(&compared, snapshot) == 0);
    assert(memcmp(snapshot->expected_tr.bytes, refs[i], 256) == 0);
  }

  memset(cpu.mtr[2], 0xa5, sizeof(cpu.mtr[2]));
  // The oldest delivered owner is generation1/A even though generation2/B has
  // the same PC/base/stride/rd. A gen2-like request must not skip A and install
  // B; only the matching generation selects its retained owner.
  assert(apply_dynamic_snapshot_event(&events[1], &snapshots[1]) != 0);
  assert(cpu.mtr[2][0]._8[0] == 0xa5);
  assert(apply_dynamic_snapshot_event(&events[0], &snapshots[1]) != 0);
  assert(cpu.mtr[2][0]._8[0] == 0xa5);
  assert(apply_dynamic_snapshot_event(&events[0], &snapshots[0]) == 0);
  assert(memcmp(cpu.mtr[2][0]._8, refs[0], 32) == 0);
  cpu.mtr[2][0]._8[0] = 0xee;
  assert(apply_dynamic_snapshot_event(&events[0], &snapshots[0]) != 0);
  assert(cpu.mtr[2][0]._8[0] == 0xee);
  assert(apply_dynamic_snapshot_event(&events[1], &snapshots[1]) == 0);
  assert(memcmp(cpu.mtr[2][0]._8, refs[1], 32) == 0);
}

static void test_shared_dynamic_then_mma(void) {
  amu_ctrl_queue_reset();
  assert(amu_snapshot_negotiate(AMU_CTRL_SNAPSHOT_VERSION,
    sizeof(amu_ctrl_snapshot_t), sizeof(amu_mma_snapshot_t),
    sizeof(amu_ctrl_event_t), TLEN / 8, ALEN / 8) == 0);
  Decode dynamic = prepare_shared_dynamic(false, false);
  prev_s = &dynamic;
  exec_mlacvt_f32_mx(&dynamic);
  uint8_t expected_tr[256];
  const uint64_t epoch = cpu.mscale_epoch;
  const uint32_t generation = cpu.mscale_generation[2];
  apply_pending_shared_dynamic(0, epoch, generation, false, expected_tr);

  // Supply an ordinary identity B operand without reloading the dynamic A
  // destination, then consume the installed TR through the real lazy MMA.
  memset(cpu.mtr[1][0]._8, 0x22, 32);
  memset(cpu.mscale[5], 127, sizeof(cpu.mscale[5]));
  cpu.mscale_valid[5] = 1;
  cpu.mscale_rows[5] = 1;
  cpu.mscale_blocks[5] = 2;
  cpu.mscale_generation[5] = 1;
  cpu.mscale_source_stride[5] = 2;
  cpu.mscale_mode[5] = 10;

  uint8_t scale_bytes[2][64];
  memset(scale_bytes, 127, sizeof(scale_bytes));
  amu_ctrl_snapshot_t scales = {0};
  scales.version = AMU_CTRL_SNAPSHOT_VERSION;
  scales.struct_size = sizeof(scales);
  for (unsigned side = 0; side < 2; ++side) {
    scales.scales[side].bytes = scale_bytes[side];
    scales.scales[side].capacity = sizeof(scale_bytes[side]);
    scales.scales[side].metadata.generation = side ? 1 : generation;
    scales.scales[side].metadata.rows = side ? 1 : 8;
    scales.scales[side].metadata.blocks = 2;
    scales.scales[side].metadata.row_pitch = 2;
    scales.scales[side].metadata.reg_id = side ? 1 : 2;
    scales.scales[side].metadata.side = side;
    scales.scales[side].metadata.valid = 1;
    scales.scales[side].metadata.source_stride = side ? 2 : 260;
  }
  for (unsigned row = 0; row < 8; ++row)
    memcpy(scale_bytes[0] + row * 2, cpu.mscale[2][row], 2);
  memcpy(scale_bytes[1], cpu.mscale[5][0], 2);

  amu_ctrl_event_t mma = {0};
  mma.valid = 1;
  mma.op = 0;
  mma.isfp = 1;
  mma.md = 4;
  mma.ms1 = 2;
  mma.ms2 = 1;
  mma.mtilem = 1;
  mma.mtilen = 1;
  mma.mtilek = 64;
  mma.types1 = 7;
  mma.types2 = 7;
  mma.typed = 2;
  mma.computeType = 10;
  mma.pc = 0x80003800;
  uint8_t matrices[4][512];
  memset(matrices, 0xa5, sizeof(matrices));
  amu_mma_snapshot_t lazy = {0};
  lazy.version = AMU_MMA_SNAPSHOT_VERSION;
  lazy.struct_size = sizeof(lazy);
  lazy.scales = &scales;
  for (unsigned i = 0; i < 4; ++i) {
    lazy.matrices[i].bytes = matrices[i];
    lazy.matrices[i].capacity = sizeof(matrices[i]);
    lazy.matrices[i].row_pitch = i < 2 ? 64 : 32;
  }
  assert(exec_amu_lazy_snapshot(&mma, &lazy) == 0);
  assert(lazy.raised_flags <= 31);
  uint32_t result;
  memcpy(&result, matrices[3], sizeof(result));
  assert(result != 0xa5a5a5a5u);
}

static void test_dynamic_convert_load(void) {
  for (unsigned side = 0; side < 2; ++side) {
    check_shared_dynamic_once(side, false);
    check_shared_dynamic_once(side, true);
  }
  test_shared_dynamic_order();
  test_shared_dynamic_preserves_young_sf();
  test_shared_dynamic_no_skip_same_pc();
  test_shared_dynamic_then_mma();
  puts("PASS SHARE_REF dynamic convert: A/B success/reject, 512 reads, local "
    "TR ownership, immutable SF/epoch/local flags, wrong/duplicate apply, "
    "older MMA/ordinary ordering, young-SF preservation, no same-PC skip, "
    "installed-TR lazy MMA");
}
#else
static void test_dynamic_convert_load(void) {
  puts("PASS dynamic converting-load feature-off/shared compile refusal");
}
#endif

static void test_shapes(void) {
  Decode d = scenario(0x18, 3, 5, 33);
  EXPECT_EXCEPTION(EX_II, mma(&d));
  d.src1.reg = 10; d.src2.reg = 11;
  load_scales(&d);
  assert(reads == (3 + 5) * 2);
  assert(cpu.mscale_rows[0] == 3 && cpu.mscale_rows[5] == 5);
  assert(cpu.mscale_blocks[0] == 2 && cpu.mscale_blocks[5] == 2);
  memset(cpu.mtr[0], 0x38, sizeof(cpu.mtr[0]));
  memset(cpu.mtr[1], 0x38, sizeof(cpu.mtr[1]));
  mma(&d); assert(macc_l32(4, 0, 0) == 0x42040000); // 33
  mma(&d); assert(macc_l32(4, 0, 0) == 0x42840000); // 66, persistent scales
  // Covered subshapes are legal, not merely exact shape matches.
  mtilem->val = 1; mtilen->val = 2; mtilek->val = 32;
  macc_l32(4, 0, 0) = 0;
  mma(&d); assert(macc_l32(4, 0, 0) == 0x42000000);
  cpu.mscale[0][0][0] = 255;
  mma(&d);
  uint32_t nan_bits = macc_l32(4, 0, 0);
  assert((nan_bits & 0x7f800000) == 0x7f800000 && (nan_bits & 0x7fffff));
  cpu.mscale[0][0][0] = 127;
  mtilem->val = 3; mtilen->val = 5; mtilek->val = 33;
  mtilem->val = 4; EXPECT_EXCEPTION(EX_II, mma(&d)); mtilem->val = 3;
  mtilen->val = 6; EXPECT_EXCEPTION(EX_II, mma(&d)); mtilen->val = 5;
  d.src1.reg = 10; d.src2.reg = 11;
  // Faulting reload preserves the entire last successful scale state.
  memory[0] = 128; fault_at = 1; d.dest.reg = 0;
  EXPECT_EXCEPTION(EX_LPF, mld_scale(&d, false));
  assert(cpu.mscale_valid[0] && cpu.mscale_rows[0] == 3 &&
    cpu.mscale_blocks[0] == 2 && cpu.mscale[0][0][0] == 127);
  fault_at = ~(vaddr_t)0;
  // Successful smaller reload replaces coverage; old bytes cannot extend it.
  mtilem->val = 1; mtilek->val = 32; mld_scale(&d, false);
  assert(cpu.mscale_rows[0] == 1 && cpu.mscale_blocks[0] == 1);
  mtilek->val = 33; EXPECT_EXCEPTION(EX_II, mma(&d));
  mtilek->val = 32; mtilem->val = 2; EXPECT_EXCEPTION(EX_II, mma(&d));
  d.src1.reg = 10; d.src2.reg = 11;
  mtilem->val = 1; load_scales(&d);
  exec_minit(&d);
  for (int i = 0; i < 8; i++) {
    assert(!cpu.mscale_valid[i] && !cpu.mscale_rows[i] && !cpu.mscale_blocks[i]);
  }
  EXPECT_EXCEPTION(EX_II, mma(&d));
  d = scenario(0x18, 1, 1, 32);
  d.dest.reg = 4; EXPECT_EXCEPTION(EX_II, mld_scale(&d, false));
  d.dest.reg = 0; mtilek->val = 0;
  EXPECT_EXCEPTION(EX_II, mld_scale(&d, false));
  mtilek->val = 32; cpu.gpr[11]._64 = 0;
  EXPECT_EXCEPTION(EX_II, mld_scale(&d, false));
  puts("PASS shapes: missing/expanded/reloaded coverage, stride, reuse, fault atomicity, minit");
}

static void test_fp4_capacity(void) {
  const unsigned capacity = 2 * TRENUM8;
  Decode d = scenario(0x16, 1, 1, capacity);
  memset(memory, 0x22, sizeof(memory));
  cpu.gpr[11]._64 = TRENUM8;
  d.dest.reg = 0;
  mld(&d, false, 'a');
  assert(reads == TRENUM8);
  for (unsigned k = 0; k < capacity; k++) assert(get_packed_mreg4(0, 0, k) == 2);
  d.dest.reg = 1; mld(&d, false, 'b');
  memset(memory, 127, sizeof(memory));
  cpu.gpr[11]._64 = AME_MAX_SCALE_BLOCKS;
  load_scales(&d);
  assert(cpu.mscale_blocks[0] == AME_MAX_SCALE_BLOCKS);
  mma(&d);
  assert(float_bits((float)capacity) == macc_l32(4, 0, 0));
  d.src1.reg = 10; d.src2.reg = 11; d.dest.reg = 0;
  mtilek->val = capacity + 1;
  EXPECT_EXCEPTION(EX_II, mld(&d, false, 'a'));
  EXPECT_EXCEPTION(EX_II, mld_scale(&d, false));
  EXPECT_EXCEPTION(EX_II, mma(&d));
  d.src1.reg = 10; d.src2.reg = 11; d.dest.reg = 0;
  mtilek->val = capacity - 1;
  reads = 0; mld(&d, false, 'a'); assert(reads == TRENUM8);
  puts("PASS FP4: max packed K, four scale blocks, odd tail, capacity+1 illegal");
}

static void test_nonuniform_and_unscaled(void) {
  Decode d = scenario(0x16, 1, 1, 32);
  for (unsigned i = 0; i < 16; i++) {
    cpu.mtr[0][0]._8[i] = (i & 1) ? 0x43 : 0x21;
    cpu.mtr[1][0]._8[i] = 0x43;
  }
  load_scales(&d);
  mma(&d);
  // 8 * ((.5*1.5 + 1*2) + (1.5*1.5 + 2*2)) = 72.
  assert(macc_l32(4, 0, 0) == 0x42900000);
  d = scenario(2, 1, 1, 32);
  cpu.mcfg[4].val = 4;
  memset(cpu.mtr[0], 1, sizeof(cpu.mtr[0]));
  memset(cpu.mtr[1], 0xfe, sizeof(cpu.mtr[1]));
  macc_l32(4, 0, 0) = 5;
  mma(&d);
  assert((int32_t)macc_l32(4, 0, 0) == -59);
  puts("PASS asymmetric packed FP4=72 and legacy unscaled INT8 C accumulation");
}

static void put_mx_lane(int reg, unsigned k, uint8_t bits, bool fp4) {
  if (fp4) set_mtreg_nibble(reg, 0, k, bits);
  else mtreg_l8(reg, 0, k) = bits;
}

// Optional export from these same checked handlers, not a second runner/model.
// A/B contain unpacked raw codes in ascending logical K. Only active lanes and
// ceil(K/32) scales are serialized; no production decoder supplies expectations.
static FILE *vector_file;
static unsigned vector_count;

static void json_string(const char *text) {
  fputc('"', vector_file);
  for (; *text; text++) {
    if (*text == '"' || *text == '\\') fputc('\\', vector_file);
    assert((unsigned char)*text >= 0x20);
    fputc(*text, vector_file);
  }
  fputc('"', vector_file);
}

static void record_mma_vector(const char *group, const char *name, uint32_t c,
    uint8_t flags_in, uint8_t flags_raised) {
  if (!vector_file) return;
  const unsigned type = cpu.mcfg[0].val;
  const bool fp4 = type == 0x16, e5m2 = type == 0x17;
  const unsigned lanes = mtilek->val, blocks = (lanes + 31) / 32;
  fprintf(vector_file, "{\"schema\":1,\"group\":");
  json_string(group);
  fprintf(vector_file, ",\"name\":");
  json_string(name);
  fprintf(vector_file, ",\"type\":%u,\"compute_type\":%u,"
    "\"is_fp4\":%s,\"is_e5m2\":%s,\"active_lanes\":%u,"
    "\"block_size\":32,\"mfrm\":0,\"a\":[",
    type, fp4 ? 10 : (e5m2 ? 8 : 7),
    fp4 ? "true" : "false", e5m2 ? "true" : "false", lanes);
  for (unsigned reg = 0; reg < 2; reg++) {
    if (reg) fprintf(vector_file, "],\"b\":[");
    for (unsigned k = 0; k < lanes; k++) {
      // Packing only: recover the exact raw byte/nibble written by the test.
      const uint8_t byte = mtreg_l8(reg, 0, fp4 ? k / 2 : k);
      const unsigned code = fp4 ? ((byte >> (4 * (k & 1))) & 15) : byte;
      fprintf(vector_file, "%s%u", k ? "," : "", code);
    }
  }
  for (unsigned side = 0; side < 2; side++) {
    fprintf(vector_file, "],\"s%c\":[", side ? 'b' : 'a');
    for (unsigned block = 0; block < blocks; block++) {
      fprintf(vector_file, "%s%u", block ? "," : "",
        cpu.mscale[side ? 5 : 0][0][block]);
    }
  }
  fprintf(vector_file, "],\"c\":\"0x%08x\",\"result\":\"0x%08x\","
    "\"flags_in\":%u,\"flags_raised\":%u,\"flags_out\":%u}\n",
    c, macc_l32(4, 0, 0), flags_in, flags_raised, (unsigned)mfflags->val);
  assert(!ferror(vector_file));
  vector_count++;
}

// Extra cases use the existing scenario/mma handlers and independent raw
// expected constants. Recheck with a preexisting DZ flag to prove stickiness.
static void check_mma_vector(Decode *d, const char *group, const char *name,
    uint32_t c, uint32_t expected, uint8_t flags) {
  for (unsigned sticky = 0; sticky <= 8; sticky += 8) {
    macc_l32(4, 0, 0) = c;
    mfflags->val = sticky;
    mcsr->val = 0xa05 | (sticky << 3);
    fflags->val = 16;
    mma(d);
    if (macc_l32(4, 0, 0) != expected ||
        mfflags->val != (flags | sticky) || fflags->val != 16 ||
        mcsr->val != (0xa05 | ((flags | sticky) << 3))) {
      fprintf(stderr, "%s: got %08x/%02x expected %08x/%02x (sticky=%u)\n",
        name, macc_l32(4, 0, 0), (unsigned)mfflags->val,
        expected, flags | sticky, sticky);
      abort();
    }
    if (!sticky) record_mma_vector(group, name, c, 0, flags);
  }
}

static void test_exact_blocks(void) {
  // All expectations are raw binary32 bits from exact dyadic identities, not
  // a second production decoder or a host floating-point reduction.
  static const struct {
    const char *name;
    unsigned type, lanes;
    uint8_t a[3], b[3], sa, sb;
    uint32_t c, result;
    uint8_t flags;
  } cases[] = {
    {"cancellation", 0x17, 3, {0x7b, 0x3c, 0xfb}, {0x7b, 0x3c, 0x7b},
      127, 127, 0, 0x3f800000, 0},
    {"full-product-lattice-cancellation", 0x17, 3, {0x7b, 0x01, 0xfb},
      {0x7b, 0x01, 0x7b}, 127, 127, 0, 0x2f800000, 0}, // 2^-32 survives
    {"one-side-overflow", 0x18, 1, {0x7e}, {0x38},
      254, 0, 0, 0x43e00000, 0}, // 448 * 2^127 * 2^-127
    {"large-products-cancel", 0x17, 2, {0x7b, 0xfb}, {0x7b, 0x7b},
      254, 254, 0x3f800000, 0x3f800000, 0},
    {"no-block-prerounding", 0x17, 2, {0x3c, 0x0c}, {0x3c, 0x0c},
      127, 127, 0xbf800000, 0x33800000, 0}, // (1 + 2^-24) - 1
    {"wide-block-C-cancel", 0x18, 1, {0x38}, {0x38},
      254, 128, 0xff7fffff, 0x73800000, 0}, // 2^128 - max_f32 = 2^104
    {"min-subnormal", 0x18, 1, {0x38}, {0x38},
      0, 105, 0, 0x00000001, 0},
    {"subnormal-half-even", 0x18, 1, {0x38}, {0x38},
      0, 104, 0, 0x00000000, 3},
    {"subnormal-half-odd", 0x18, 1, {0x3c}, {0x38},
      0, 105, 0, 0x00000002, 3}, // 3 * 2^-150
    {"negative-underflow", 0x18, 1, {0xb8}, {0x38},
      0, 104, 0, 0x80000000, 3},
    {"normal-result-underflow", 0x17, 2, {0x3c, 0x0c}, {0x3c, 0x8c},
      0, 128, 0, 0x00800000, 3}, // Tiny p=24 result before subnormal rounding.
    {"extreme-small-scale", 0x17, 1, {0x01}, {0x01},
      0, 0, 0, 0, 3}, // 2^-286
    {"overflow", 0x18, 1, {0x38}, {0x38},
      254, 128, 0, 0x7f800000, 5},
    {"negative-overflow", 0x18, 1, {0xb8}, {0x38},
      254, 254, 0, 0xff800000, 5},
    {"tie-even", 0x18, 1, {0x38}, {0x38},
      103, 127, 0x3f800000, 0x3f800000, 1},
    {"tie-odd", 0x18, 1, {0x38}, {0x38},
      103, 127, 0x3f800001, 0x3f800002, 1},
    {"negative-tie-odd", 0x18, 1, {0xb8}, {0x38},
      103, 127, 0xbf800001, 0xbf800002, 1},
    {"double-round-above", 0x17, 2, {0x3c, 0x0c}, {0x3c, 0x0c},
      127, 127, 0x00000001, 0x3f800001, 1}, // midpoint + 2^-149
    {"double-round-below", 0x17, 2, {0x3c, 0x0c}, {0x3c, 0x0c},
      127, 127, 0x80000001, 0x3f800000, 1},
    {"double-round-odd-below", 0x17, 2, {0x3c, 0x0e}, {0x3c, 0x10},
      127, 127, 0x80000001, 0x3f800001, 1}, // 1 + 3*2^-24 - 2^-149
    {"negative-zero", 0x16, 1, {0x08}, {0x02},
      254, 254, 0x80000000, 0x80000000, 0},
    {"mixed-zero", 0x16, 1, {0x08}, {0x02},
      254, 254, 0x00000000, 0x00000000, 0},
    {"positive-zero", 0x18, 1, {0x80}, {0xb8},
      0, 0, 0x80000000, 0x00000000, 0},
    {"exact-zero", 0x16, 2, {0x02, 0x0a}, {0x02, 0x02},
      254, 254, 0x80000000, 0x00000000, 0},
    {"scale-A-NaN", 0x16, 1, {0}, {0x02},
      255, 127, 0, 0x7fc00000, 0},
    {"scale-B-NaN", 0x18, 1, {0x38}, {0x38},
      127, 255, 0, 0x7fc00000, 0},
    {"E4M3-NaN", 0x18, 1, {0x7f}, {0x38},
      127, 127, 0, 0x7fc00000, 0},
    {"E5M2-qNaN", 0x17, 1, {0x7e}, {0x3c},
      127, 127, 0, 0x7fc00000, 0},
    {"E5M2-sNaN", 0x17, 1, {0x7d}, {0x3c},
      127, 127, 0, 0x7fc00000, 16},
    {"infinity", 0x17, 1, {0x7c}, {0x3c},
      0, 0, 0, 0x7f800000, 0},
    {"zero-times-infinity", 0x17, 1, {0}, {0x7c},
      127, 127, 0, 0x7fc00000, 16},
    {"opposite-infinities", 0x17, 2, {0x7c, 0xfc}, {0x3c, 0x3c},
      127, 127, 0, 0x7fc00000, 16},
    {"C-opposite-infinity", 0x17, 1, {0x7c}, {0x3c},
      127, 127, 0xff800000, 0x7fc00000, 16},
    {"C-sNaN", 0x18, 1, {0x38}, {0x38},
      127, 127, 0x7f800001, 0x7fc00000, 16},
    {"C-qNaN", 0x18, 1, {0x38}, {0x38},
      127, 127, 0x7fc00001, 0x7fc00000, 0},
  };
  for (unsigned i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
    const bool fp4 = cases[i].type == 0x16, e5m2 = cases[i].type == 0x17;
    // Matrix operations must not leak into the scalar SoftFloat environment.
    softfloat_roundingMode = softfloat_round_max;
    softfloat_exceptionFlags = softfloat_flag_invalid;
    softfloat_detectTininess = softfloat_tininess_beforeRounding;
    uint8_t flags = 8;
    uint32_t bits = mxfp_accumulate_block(cases[i].c, cases[i].a, cases[i].b,
      cases[i].lanes, cases[i].sa, cases[i].sb, fp4, e5m2, &flags);
    if (bits != cases[i].result || flags != (cases[i].flags | 8)) {
      fprintf(stderr, "%s: got %08x/%02x expected %08x/%02x\n",
        cases[i].name, bits, flags, cases[i].result, cases[i].flags | 8);
      abort();
    }
    assert(softfloat_roundingMode == softfloat_round_max &&
      softfloat_exceptionFlags == softfloat_flag_invalid &&
      softfloat_detectTininess == softfloat_tininess_beforeRounding);
    // Independently distinguish newly raised flags from the sticky DZ bit.
    uint8_t clean_flags = 0;
    assert(mxfp_accumulate_block(cases[i].c, cases[i].a, cases[i].b,
      cases[i].lanes, cases[i].sa, cases[i].sb, fp4, e5m2, &clean_flags) ==
      cases[i].result);
    assert(clean_flags == cases[i].flags);

    // The real handler must call the same golden and publish only mfflags.
    Decode d = scenario(cases[i].type, 1, 1, cases[i].lanes);
    load_scales(&d);
    cpu.mscale[0][0][0] = cases[i].sa;
    cpu.mscale[5][0][0] = cases[i].sb;
    for (unsigned k = 0; k < cases[i].lanes; k++) {
      put_mx_lane(0, k, cases[i].a[k], fp4);
      put_mx_lane(1, k, cases[i].b[k], fp4);
    }
    macc_l32(4, 0, 0) = cases[i].c;
    mfflags->val = 8; fflags->val = 16;
    mma(&d);
    assert(macc_l32(4, 0, 0) == cases[i].result);
    assert(mfflags->val == (cases[i].flags | 8) && fflags->val == 16);
    assert(softfloat_roundingMode == softfloat_round_max &&
      softfloat_exceptionFlags == softfloat_flag_invalid &&
      softfloat_detectTininess == softfloat_tininess_beforeRounding);
    record_mma_vector("frozen35", cases[i].name, cases[i].c, 8, clean_flags);
  }
  printf("PASS exact blocks: %zu raw-bit cases, handler/flags/state-restore\n",
    sizeof(cases) / sizeof(cases[0]));
}

static void test_block_order_and_rounding(void) {
  static const unsigned types[] = {0x16, 0x17, 0x18};
  static const uint8_t one[] = {2, 0x3c, 0x38};
  static const char *const formats[] = {"FP4", "E5M2", "E4M3"};
  unsigned checked = 0;
  for (unsigned t = 0; t < 3; t++) {
    char name[80];
    Decode d = scenario(types[t], 1, 1, 33);
    load_scales(&d);
    // Each block contributes half an ulp to C=1. RNE after each block is 1,
    // unlike rounding the complete K reduction once (which would be 1+ulp).
    cpu.mscale[0][0][0] = cpu.mscale[0][0][1] = 103;
    for (unsigned k = 0; k < 33; k++) {
      put_mx_lane(0, k, k == 0 || k == 32 ? one[t] : 0, t == 0);
      put_mx_lane(1, k, one[t], t == 0);
    }
    // Poison inactive lanes and scale blocks; K33 must never consume them.
    if (t) {
      memset(&cpu.mtr[0][0]._8[33], 0x7f, TRENUM8 - 33);
      memset(&cpu.mtr[1][0]._8[33], 0x7f, TRENUM8 - 33);
    } else {
      set_mtreg_nibble(0, 0, 33, 7);
      set_mtreg_nibble(1, 0, 33, 7);
    }
    for (unsigned b = 2; b < AME_MAX_SCALE_BLOCKS; b++) {
      cpu.mscale[0][0][b] = cpu.mscale[5][0][b] = 255;
    }
    snprintf(name, sizeof(name), "%s K33 ordered two half-ulp blocks", formats[t]);
    check_mma_vector(&d, "ordering9", name, 0x3f800000, 0x3f800000, 1);
    checked++;
    // K order matters: (2^25 - 2^25) + 1 = 1; reversed blocks yield 0.
    put_mx_lane(0, 0, one[t] | (t == 0 ? 8 : 128), t == 0);
    cpu.mscale[0][0][0] = 152;
    cpu.mscale[0][0][1] = 127;
    snprintf(name, sizeof(name), "%s K33 ascending-K cancellation", formats[t]);
    check_mma_vector(&d, "ordering9", name, 0x4c000000, 0x3f800000, 0);
    checked++;
    // Finite contributions +2^128 then -2^128: first C insertion overflows,
    // then +Inf C plus a finite negative contribution stays +Inf (not NaN).
    mtilek->val = 64;
    for (unsigned k = 0; k < 64; k++) {
      put_mx_lane(0, k, k == 0 ? one[t] :
        (k == 32 ? one[t] | (t == 0 ? 8 : 128) : 0), t == 0);
      put_mx_lane(1, k, one[t], t == 0);
    }
    cpu.mscale[0][0][0] = cpu.mscale[0][0][1] = 254;
    cpu.mscale[5][0][0] = cpu.mscale[5][0][1] = 128;
    snprintf(name, sizeof(name), "%s K64 ordered overflowing opposite blocks", formats[t]);
    check_mma_vector(&d, "ordering9", name, 0, 0x7f800000, 5);
    checked++;
    for (unsigned rm = 1; rm < 8; rm++) {
      mfrm->val = rm;
      EXPECT_EXCEPTION(EX_II, mma(&d));
      assert(macc_l32(4, 0, 0) == 0x7f800000 && mfflags->val == (5 | 8));
    }
  }
  assert(checked == 9);
  printf("PASS K33/K64: %u raw-bit cases, ordered rounding/overflow, "
    "inactive poison, 21 non-RNE rejections\n", checked);
}

static void test_cute_raw_goldens(void) {
  // Exact 256-bit carriers from FPeFp4ProbeSpec, not short-lane substitutes:
  // FP4 consumes 64 lanes/two MX32 blocks; FP8 consumes 32/one. Omitted
  // codes are +0 padding. Expected values/flags are independent dyadic
  // identities (the same frozen contract as frozen35), never DUT output.
  static const struct {
    const char *name;
    unsigned type;
    uint8_t a[64], b[64], sa[2], sb[2];
    uint32_t c, result;
    uint8_t flags;
  } cases[] = {
    {"FP4 low/high consecutive K = 44", 0x16, {0}, {0},
      {127, 127}, {127, 127}, 0, 0x42300000, 0},
    {"FP4 swapped A nibbles = 40", 0x16, {0}, {0},
      {127, 127}, {127, 127}, 0, 0x42200000, 0},
    {"FP4 alternating bytes = 72", 0x16, {0}, {0},
      {127, 127}, {127, 127}, 0, 0x42900000, 0},
    {"FP4 odd tie", 0x16, {2}, {2},
      {103, 127}, {127, 127}, 0x3f800001, 0x3f800002, 1},
    {"FP4 tiny negative addend", 0x16, {10}, {2},
      {0, 127}, {0, 127}, 0x3f800000, 0x3f800000, 1},
    {"FP4 ordered two half-ulp blocks", 0x16, {[0] = 2, [32] = 2}, {0},
      {103, 103}, {127, 127}, 0x3f800000, 0x3f800000, 1},
    {"FP4 opposite half-ulp blocks", 0x16, {[0] = 2, [32] = 10}, {0},
      {103, 103}, {127, 127}, 0x3f800000, 0x3f7fffff, 1},
    {"FP4 ordered overflowing opposite blocks", 0x16, {[0] = 2, [32] = 10}, {0},
      {254, 254}, {128, 128}, 0, 0x7f800000, 5},
    {"E5 exact cancellation", 0x17, {0x7b, 0x3c, 0xfb}, {0x7b, 0x3c, 0x7b},
      {127}, {127}, 0, 0x3f800000, 0},
    {"E5 full product lattice cancellation", 0x17, {0x7b, 0x01, 0xfb},
      {0x7b, 0x01, 0x7b}, {127}, {127}, 0, 0x2f800000, 0},
    {"E5 do not preround block before C", 0x17, {0x3c, 0x0c}, {0x3c, 0x0c},
      {127}, {127}, 0xbf800000, 0x33800000, 0},
    {"E5 midpoint plus minsub C", 0x17, {0x3c, 0x0c}, {0x3c, 0x0c},
      {127}, {127}, 0x00000001, 0x3f800001, 1},
    {"E5 midpoint minus minsub C", 0x17, {0x3c, 0x0c}, {0x3c, 0x0c},
      {127}, {127}, 0x80000001, 0x3f800000, 1},
    {"E4 extreme paired scales stay finite", 0x18, {0x7e}, {0x38},
      {254}, {0}, 0, 0x43e00000, 0},
    {"E4 wide contribution cancels finite C", 0x18, {0x38}, {0x38},
      {254}, {128}, 0xff7fffff, 0x73800000, 0},
    {"E4 minsub exact", 0x18, {0x38}, {0x38},
      {0}, {105}, 0, 0x00000001, 0},
    {"E4 half minsub RNE even", 0x18, {0x38}, {0x38},
      {0}, {104}, 0, 0x00000000, 3},
    {"E4 half minsub negative zero", 0x18, {0xb8}, {0x38},
      {0}, {104}, 0, 0x80000000, 3},
    {"E4 odd tie", 0x18, {0x38}, {0x38},
      {103}, {127}, 0x3f800001, 0x3f800002, 1},
    {"E5 odd tie", 0x17, {0x3c}, {0x3c},
      {103}, {127}, 0x3f800001, 0x3f800002, 1},
    {"FP4 NaN A scale with zero data", 0x16, {0}, {2},
      {255, 127}, {127, 127}, 0, 0x7fc00000, 0},
    {"E4 NaN B scale", 0x18, {0x38}, {0x38},
      {127}, {255}, 0, 0x7fc00000, 0},
  };
  _Static_assert(sizeof(cases) / sizeof(cases[0]) == 22, "CUTE raw vector count");
  for (unsigned i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
    const bool fp4 = cases[i].type == 0x16;
    const unsigned lanes = fp4 ? 64 : 32;
    Decode d = scenario(cases[i].type, 1, 1, lanes);
    load_scales(&d);
    for (unsigned block = 0; block < (lanes + 31) / 32; block++) {
      cpu.mscale[0][0][block] = cases[i].sa[block];
      cpu.mscale[5][0][block] = cases[i].sb[block];
    }
    for (unsigned k = 0; k < lanes; k++) {
      uint8_t a = cases[i].a[k], b = cases[i].b[k];
      if (i < 3 && k < 32) {
        a = i == 0 ? 1 + k % 2 : (i == 1 ? 2 - k % 2 : 1 + k % 4);
        b = 3 + k % 2;
      } else if (i >= 5 && i <= 7) {
        b = 2;
      }
      put_mx_lane(0, k, a, fp4);
      put_mx_lane(1, k, b, fp4);
    }
    check_mma_vector(&d, "cute22", cases[i].name,
      cases[i].c, cases[i].result, cases[i].flags);
  }
  puts("PASS CUTE raw goldens: 22 full-carrier cases, result/flags/stickiness");
}

static void test_scale_ff_nodot(void) {
  static const unsigned types[] = {0x17, 0x18, 0x16};
  static const char *const formats[] = {"E5M2", "E4M3", "FP4"};
  static const uint8_t one[] = {0x3c, 0x38, 2};
  // E5 poison includes sNaN and Inf*0 on BOTH operands. E4 has quiet NaNs;
  // FP4 has finite signed maxima. Expectations never come from a decoder.
  static const uint8_t poison[] = {0x7d, 0x7f, 7};
  static const uint8_t poison_second[] = {0x7c, 0x7e, 15};
  static const char *const sides[] = {"A", "B", "AB"};
  static const struct { const char *name; uint32_t bits; uint8_t flags; } cs[] = {
    {"finite-C", 0x3f800000, 0}, {"positive-Inf-C", 0x7f800000, 0},
    {"negative-Inf-C", 0xff800000, 0}, {"qNaN-C", 0x7fc00001, 0},
    {"sNaN-C", 0x7f800001, 16},
  };
  static const struct {
    const char *name; uint8_t sa, sb, flags; uint32_t c;
  } prior[] = {
    {"OF-NX", 254, 128, 5, 0},
    {"UF-NX", 0, 104, 3, 0},
    {"NX", 103, 127, 1, 0x3f800000},
  };
  unsigned checked = 0;
  for (unsigned t = 0; t < 3; t++) {
    const bool fp4 = types[t] == 0x16;
    char name[100];
    for (unsigned side = 0; side < 3; side++) {
      const uint8_t sa = side == 1 ? 127 : 255;
      const uint8_t sb = side == 0 ? 127 : 255;
      for (unsigned ci = 0; ci < sizeof(cs) / sizeof(cs[0]); ci++) {
        Decode d = scenario(types[t], 1, 1, 32);
        load_scales(&d);
        cpu.mscale[0][0][0] = sa;
        cpu.mscale[5][0][0] = sb;
        for (unsigned k = 0; k < 32; k++) {
          put_mx_lane(0, k, k == 0 ? poison[t] :
            (k == 1 ? poison_second[t] : 0), fp4);
          put_mx_lane(1, k, k == 2 ? poison[t] :
            (k == 3 ? poison_second[t] : 0), fp4);
        }
        snprintf(name, sizeof(name), "%s scale-%s-FF %s no-dot",
          formats[t], sides[side], cs[ci].name);
        check_mma_vector(&d, "scaleFF", name, cs[ci].bits, 0x7fc00000, cs[ci].flags);
        // A disabled dot must not even dereference element arrays. Use NULL
        // only for this helper-level check, not architectural load semantics.
        softfloat_roundingMode = softfloat_round_max;
        softfloat_exceptionFlags = softfloat_flag_invalid;
        softfloat_detectTininess = softfloat_tininess_beforeRounding;
        uint8_t flags = 7; // Existing OF/UF/NX must survive the early branch.
        assert(mxfp_accumulate_block(cs[ci].bits, NULL, NULL, 32,
          sa, sb, fp4, types[t] == 0x17, &flags) == 0x7fc00000);
        assert(flags == (7 | cs[ci].flags));
        assert(softfloat_roundingMode == softfloat_round_max &&
          softfloat_exceptionFlags == softfloat_flag_invalid &&
          softfloat_detectTininess == softfloat_tininess_beforeRounding);
        checked++;
      }
    }
    for (unsigned pi = 0; pi < sizeof(prior) / sizeof(prior[0]); pi++) {
      for (unsigned side = 0; side < 2; side++) {
        Decode d = scenario(types[t], 1, 1, 64);
        load_scales(&d);
        cpu.mscale[0][0][0] = prior[pi].sa;
        cpu.mscale[5][0][0] = prior[pi].sb;
        cpu.mscale[0][0][1] = side == 0 ? 255 : 127;
        cpu.mscale[5][0][1] = side == 1 ? 255 : 127;
        for (unsigned k = 0; k < 64; k++) {
          put_mx_lane(0, k, k == 0 ? one[t] :
            (k == 32 ? poison[t] : (k == 33 ? poison_second[t] : 0)), fp4);
          put_mx_lane(1, k, k == 0 ? one[t] :
            (k == 34 ? poison[t] : (k == 35 ? poison_second[t] : 0)), fp4);
        }
        snprintf(name, sizeof(name), "%s %s then scale-%s-FF",
          formats[t], prior[pi].name, sides[side]);
        check_mma_vector(&d, "scaleFF", name, prior[pi].c, 0x7fc00000, prior[pi].flags);
        checked++;
      }
    }
    // No-dot is per block, not an instruction-wide early return: a later
    // finite-scale E5 block still evaluates sNaN/Inf*0 and raises NV.
    Decode d = scenario(types[t], 1, 1, 64);
    load_scales(&d);
    cpu.mscale[0][0][0] = 255;
    for (unsigned k = 0; k < 64; k++) {
      put_mx_lane(0, k, k == 0 || k == 32 ? poison[t] :
        (k == 1 || k == 33 ? poison_second[t] : 0), fp4);
      put_mx_lane(1, k, k == 2 || k == 34 ? poison[t] :
        (k == 3 || k == 35 ? poison_second[t] : 0), fp4);
    }
    snprintf(name, sizeof(name), "%s scale-A-FF then finite-scale poison", formats[t]);
    check_mma_vector(&d, "scaleFF", name, 0, 0x7fc00000, types[t] == 0x17 ? 16 : 0);
    checked++;
  }
  assert(checked == 66);
  printf("PASS scale-FF no-dot: %u directed cases (45 C/side +18 prior-sticky "
    "+3 later-block), 45 NULL-array/state-restore checks\n", checked);
}

#ifdef NEMU_MXFP_QUEUE_TEST
static void test_snapshot_admission(void) {
  const uint32_t valid[6] = {AMU_CTRL_SNAPSHOT_VERSION, sizeof(amu_ctrl_snapshot_t),
    sizeof(amu_mma_snapshot_t), sizeof(amu_ctrl_event_t), TLEN / 8, ALEN / 8};
  amu_snapshot_reset();
  assert(!amu_snapshot_ready());
  assert(amu_snapshot_negotiate(1, sizeof(amu_ctrl_snapshot_t), sizeof(amu_mma_snapshot_t),
    sizeof(amu_ctrl_event_t), TLEN / 8, ALEN / 8) == -2);
  for (unsigned field = 0; field < 6; ++field) {
    uint32_t wrong[6];
    memcpy(wrong, valid, sizeof(wrong));
    wrong[field]++;
    assert(amu_snapshot_negotiate(wrong[0], wrong[1], wrong[2], wrong[3], wrong[4], wrong[5]) == -2);
    assert(!amu_snapshot_ready());
    assert(amu_snapshot_negotiate(valid[0], valid[1], valid[2], valid[3], valid[4], valid[5]) == 0);
    assert(amu_snapshot_ready());
  }
  amu_snapshot_reset();
  assert(!amu_snapshot_ready());
  assert(amu_snapshot_negotiate(valid[0], valid[1], valid[2], valid[3], valid[4], valid[5]) == 0);
  puts("PASS explicit snapshot admission: version/sizes/prefix/capacities refusals6, reset, idempotent");
}

static void test_active_load_compare(void) {
  static const unsigned modes[] = {0, 7};
  static const unsigned lengths[] = {1, 3, 33, 64};
  uint8_t *observed = malloc(TLEN / 8);
  assert(observed);
  unsigned checked = 0;
  for (unsigned mode = 0; mode < 2; ++mode) {
    for (unsigned length = 0; length < 4; ++length) {
      for (unsigned side = 0; side < 2; ++side) {
        const unsigned k = lengths[length], rows = 5;
        const unsigned bytes = modes[mode] == 7 ? (k + 1) / 2 : k;
        amu_ctrl_event_t event = {.op = 1, .md = 2, .types2 = side,
          .typed = modes[mode], .computeType = 0,
          .mtilem = side ? rows : k, .mtilen = side ? k : rows,
          .stride = 65, .pc = 0x80001264};
        memset(cpu.mtr[2], 0xa5, TLEN / 8);
        memset(observed, 0x5a, TLEN / 8);
        for (unsigned row = 0; row < rows; ++row) {
          for (unsigned col = 0; col < bytes; ++col) {
            memory[row * event.stride + col] = (uint8_t)(0x7e + row + col);
            observed[row * (TRLEN / 8) + col] = memory[row * event.stride + col];
          }
          if (modes[mode] == 7 && (k & 1)) {
            observed[row * (TRLEN / 8) + bytes - 1] ^= 0xf0;
          }
        }
        allow_matrix_memory = true;
        assert(exec_amu(&event, observed) == 0); // Different unobserved tails.
        if (modes[mode] == 7 && (k & 1)) {
          for (unsigned row = 0; row < rows; ++row) {
            assert((cpu.mtr[2][row]._8[bytes - 1] & 0xf0) == 0xa0);
          }
        }
        observed[0] ^= 1;
        assert(exec_amu(&event, observed) != 0);
        observed[0] ^= 1;
        const unsigned last = (rows - 1) * (TRLEN / 8) + bytes - 1;
        observed[last] ^= 1;
        assert(exec_amu(&event, observed) != 0); // Includes odd FP4 low nibble.
        observed[last] ^= 1;
        allow_matrix_memory = false;
        assert(exec_amu(&event, NULL) != 0);
        event.mtilem = event.mtilen = 0;
        assert(exec_amu(&event, observed) != 0); // Reject before memory access.
        checked++;
      }
    }
  }
  // Whole e8 events encode the full physical shape: active comparison must
  // remain a full-register comparison, including its very last byte.
  amu_ctrl_event_t whole = {.op = 1, .md = 2, .types2 = 1,
    .mtilem = ROWNUM, .mtilen = TRLEN / 8, .stride = TRLEN / 8};
  for (unsigned i = 0; i < TLEN / 8; ++i) memory[i] = observed[i] = (uint8_t)i;
  allow_matrix_memory = true;
  assert(exec_amu(&whole, observed) == 0);
  observed[TLEN / 8 - 1] ^= 1;
  assert(exec_amu(&whole, observed) != 0);
  allow_matrix_memory = false;
  amu_ctrl_event_t zero = {.op = 3, .md = 2};
  memset(observed, 0, TLEN / 8);
  assert(exec_amu(&zero, observed) == 0);
  observed[TLEN / 8 - 1] = 1;
  assert(exec_amu(&zero, observed) != 0); // MARITH retains full comparison.
  free(observed);
  printf("PASS normal MX MLS active compare: %u geometry/side cases, "
    "inactive bytes ignored, odd high nibble preserved, active corruptions%u rejected\n",
    checked, checked * 2);
  puts("PASS whole-shape and MARITH full-register last-byte corruptions rejected");
}

static void test_owned_scale_queue(void) {
  Decode d = scenario(0x16, 3, 5, 33);
  d.pc = 0x80000000;
  prev_s = &d;
  amu_ctrl_queue_reset();
  load_scales(&d);
  assert(cpu.mscale_generation[0] == 1 && cpu.mscale_generation[5] == 1);
#ifdef CONFIG_DIFFTEST_AMU_CTRL
  // The real shared configuration auto-enqueues each scale load. Consume the
  // two setup owners through the production snapshot path before installing
  // the explicit transport owners this foundation test is designed to check.
  static const unsigned setup_rows[2] = {3, 5};
  for (unsigned side = 0; side < 2; ++side) {
    const unsigned live_bytes = setup_rows[side] * 2;
    uint8_t setup[2][16];
    memset(setup, 0xa5, sizeof(setup));
    amu_ctrl_snapshot_t setup_snapshot = {0};
    setup_snapshot.version = AMU_CTRL_SNAPSHOT_VERSION;
    setup_snapshot.struct_size = sizeof(setup_snapshot);
    for (unsigned s = 0; s < 2; ++s) {
      setup_snapshot.scales[s].bytes = setup[s];
      setup_snapshot.scales[s].capacity = sizeof(setup[s]);
    }
    amu_ctrl_event_t setup_event = amu_ctrl_queue_front();
    assert(setup_event.op == 1 && setup_event.isScale &&
      setup_event.types2 == (side == 0) && setup_event.md == side &&
      setup_event.mtilem == setup_rows[side] && setup_event.mtilen == 2);
    amu_ctrl_event_t setup_compared = setup_event;
    assert(check_amu_ctrl_snapshot(&setup_compared, &setup_snapshot) == 0);
    const amu_scale_metadata_t *setup_meta = &setup_snapshot.scales[side].metadata;
    assert(setup_snapshot.epoch == 0 && setup_meta->valid &&
      setup_meta->generation == 1 && setup_meta->rows == setup_rows[side] &&
      setup_meta->blocks == 2 && setup_meta->row_pitch == 2 &&
      setup_meta->reg_id == side && setup_meta->side == side &&
      setup_meta->source_stride == 5);
    for (unsigned i = 0; i < live_bytes; ++i) assert(setup[side][i] == 127);
    assert(setup[side][live_bytes] == 0xa5);
    assert(!setup_snapshot.scales[!side].metadata.valid);
  }
  assert(amu_ctrl_queue_empty());
#endif
  amu_ctrl_event_t load = {0};
  load.valid = 1; load.op = 1; load.isScale = 1; load.types2 = 1;
  load.mtilem = 3; load.mtilen = 2; load.stride = 5; load.computeType = 10;
  load.pc = d.pc;
  amu_ctrl_queue_push(load, 0, NULL);
  // Freeze a second entry's MMA binding, then replace both live scale states.
  amu_ctrl_queue_mma_emplace(4, 0, false, true, 0, 1, 3, 5, 33, 7, 7, 2, 10);
  memory[0] = 90;
  d.dest.reg = 0;
  mld_scale(&d, false);
  assert(cpu.mscale_generation[0] == 2);
  exec_minit(&d);
  assert(cpu.mscale_epoch == 1 && !cpu.mscale_valid[0] && !cpu.mscale_generation[0]);
  mld_scale(&d, false);
  assert(cpu.mscale_generation[0] == 1 && cpu.mscale_epoch == 1);
#ifdef CONFIG_DIFFTEST_AMU_CTRL
  const unsigned expected_pending = 4;
#else
  const unsigned expected_pending = 2;
#endif

  uint8_t a[16], b[16];
  memset(a, 0xcc, sizeof(a)); memset(b, 0xdd, sizeof(b));
  amu_ctrl_snapshot_t snapshot = {0};
  snapshot.version = AMU_CTRL_SNAPSHOT_VERSION;
  snapshot.struct_size = sizeof(snapshot);
  snapshot.scales[0].bytes = a; snapshot.scales[0].capacity = sizeof(a);
  snapshot.scales[1].bytes = b; snapshot.scales[1].capacity = sizeof(b);
  amu_ctrl_event_t compared = load;
  snapshot.version++;
  assert(check_amu_ctrl_snapshot(&compared, &snapshot) == -2);
  assert(amu_ctrl_queue_size() == expected_pending && a[0] == 0xcc);
  snapshot.version--;
  snapshot.struct_size--;
  assert(check_amu_ctrl_snapshot(&compared, &snapshot) == -2);
  snapshot.struct_size++;
  snapshot.scales[0].capacity = 5; // Six active A scale bytes.
  assert(check_amu_ctrl_snapshot(&compared, &snapshot) == -2);
  assert(amu_ctrl_queue_size() == expected_pending && a[0] == 0xcc);
  snapshot.scales[0].capacity = sizeof(a);
  assert(check_amu_ctrl_snapshot(&compared, &snapshot) == 0);
  assert(snapshot.epoch == 0 && snapshot.scales[0].metadata.generation == 1);
  assert(snapshot.scales[0].metadata.rows == 3 && snapshot.scales[0].metadata.blocks == 2);
  assert(snapshot.scales[0].metadata.source_stride == 5);
  assert(snapshot.scales[0].metadata.row_pitch == 2);
  for (unsigned i = 0; i < 6; ++i) assert(a[i] == 127);
  assert(a[6] == 0xcc && !snapshot.scales[1].metadata.valid);

  compared = amu_ctrl_queue_front();
  snapshot.scales[1].capacity = 9; // Ten active B scale bytes.
  a[0] = 0xcc;
  assert(check_amu_ctrl_snapshot(&compared, &snapshot) == -2);
  assert(amu_ctrl_queue_size() == expected_pending - 1 && a[0] == 0xcc);
  snapshot.scales[1].capacity = sizeof(b);
  snapshot.scales[1].bytes = NULL;
  assert(check_amu_ctrl_snapshot(&compared, &snapshot) == -2);
  snapshot.scales[1].bytes = b;
  assert(check_amu_ctrl_snapshot(&compared, &snapshot) == 0);
  assert(snapshot.epoch == 0 && snapshot.scales[0].metadata.generation == 1);
  assert(snapshot.scales[1].metadata.generation == 1);
  assert(snapshot.scales[1].metadata.reg_id == 1 && snapshot.scales[1].metadata.side == 1);
  for (unsigned i = 0; i < 6; ++i) assert(a[i] == 127);
  for (unsigned i = 0; i < 10; ++i) assert(b[i] == 127);
  assert(b[10] == 0xdd);
#ifdef CONFIG_DIFFTEST_AMU_CTRL
  // The post-reload and post-MINIT A scale writes are also real owners. Keep
  // their young live-SF provenance distinct: generation2/epoch0 then
  // generation1/epoch1, with the same real bytes, and drain both explicitly.
  const struct {
    uint64_t epoch;
    uint32_t generation;
  } young[] = {{0, 2}, {1, 1}};
  for (unsigned i = 0; i < 2; ++i) {
    uint8_t young_bytes[2][16];
    memset(young_bytes, 0xa5, sizeof(young_bytes));
    amu_ctrl_snapshot_t young_snapshot = {0};
    young_snapshot.version = AMU_CTRL_SNAPSHOT_VERSION;
    young_snapshot.struct_size = sizeof(young_snapshot);
    for (unsigned s = 0; s < 2; ++s) {
      young_snapshot.scales[s].bytes = young_bytes[s];
      young_snapshot.scales[s].capacity = sizeof(young_bytes[s]);
    }
    amu_ctrl_event_t young_event = amu_ctrl_queue_front();
    assert(young_event.op == 1 && young_event.isScale &&
      young_event.md == 0 && young_event.types2 == 1);
    compared = young_event;
    assert(check_amu_ctrl_snapshot(&compared, &young_snapshot) == 0);
    const amu_scale_metadata_t *young_meta = &young_snapshot.scales[0].metadata;
    assert(young_snapshot.epoch == young[i].epoch && young_meta->valid &&
      young_meta->generation == young[i].generation && young_meta->rows == 3 &&
      young_meta->blocks == 2 && young_meta->row_pitch == 2 &&
      young_meta->reg_id == 0 && young_meta->side == 0 &&
      young_meta->source_stride == 5);
    assert(young_bytes[0][0] == 90);
    for (unsigned byte = 1; byte < 6; ++byte) assert(young_bytes[0][byte] == 127);
    assert(young_bytes[0][6] == 0xa5);
    assert(!young_snapshot.scales[1].metadata.valid);
  }
#endif
  assert(amu_ctrl_queue_empty());
  assert(check_amu_ctrl_snapshot(&compared, &snapshot) == -1);
  puts("PASS owned REF scale queue: auto/manual owners, reload/MINIT immutable, "
    "side/shape/stride/epoch/generation, ABI/capacity rejects5");
}

static void test_owned_dynamic_queue(void) {
  // Transport/ownership test, not a second quantizer. The producer's numeric
  // results are separately checked by test_dynamic_convert_load.
  assert(amu_snapshot_negotiate(AMU_CTRL_SNAPSHOT_VERSION,
    sizeof(amu_ctrl_snapshot_t), sizeof(amu_mma_snapshot_t),
    sizeof(amu_ctrl_event_t), TLEN / 8, ALEN / 8) == 0);
  unsigned checked = 0;
  for (unsigned side = 0; side < 2; ++side) {
    for (unsigned rejected = 0; rejected < 2; ++rejected) {
      Decode d = scenario(0x16, 8, 8, 64);
      d.pc = 0x80001800;
      prev_s = &d;
      amu_ctrl_queue_reset();
      const unsigned reg = side + 1, index = side * 4 + reg;
      amu_ctrl_event_t event = {.valid = 1, .op = 1, .md = reg,
        .types2 = !side, .typed = 2, .base = 256, .stride = 256,
        .mtilem = side ? 64 : 8, .mtilen = side ? 8 : 64,
        .pc = d.pc, .computeType = 10, .isConvertF32 = 1, .isDynamicMxfpQuant = 1};
      uint8_t expected_tr[256], expected_sf[16];
      for (unsigned row = 0; row < 8; ++row) {
        for (unsigned byte = 0; byte < 32; ++byte)
          expected_tr[row * 32 + byte] =
            (uint8_t)(row * 13 + byte + side);
        for (unsigned block = 0; block < 2; ++block)
          cpu.mscale[index][row][block] = expected_sf[row * 2 + block] =
            (uint8_t)(row * 2 + block + 120);
      }
      // Missing zero-valued data is still covered by the C++ observer gate;
      // the REF queue must retain it verbatim, not use zero as "not present".
      cpu.mtr[reg][7]._8[31] = expected_tr[255] = 0;
      cpu.mscale[index][7][1] = expected_sf[15] = 0;
      cpu.mscale_valid[index] = !rejected;
      cpu.mscale_rows[index] = 8;
      cpu.mscale_blocks[index] = 2;
      cpu.mscale_generation[index] = 9;
      cpu.mscale_source_stride[index] = 256;
      cpu.mscale_mode[index] = 10;
      mfflags->val = 16; // The success owner must nevertheless snapshot local0.
      amu_matrix_view_t ref_result = {expected_tr, sizeof(expected_tr), 32, 0};
      amu_ctrl_queue_push(event, rejected ? 16 : 0, &ref_result);
      memset(cpu.mtr[reg], 0xe5, sizeof(cpu.mtr[reg]));
      memset(cpu.mscale[index], 0xff, sizeof(cpu.mscale[index]));
      exec_minit(&d);
      assert(cpu.mscale_epoch == 1 && !cpu.mscale_generation[index]);

      uint8_t tr[257], sf[2][17];
      memset(tr, 0xa5, sizeof(tr));
      memset(sf, 0xa5, sizeof(sf));
      amu_ctrl_snapshot_t snapshot = {0};
      snapshot.version = AMU_CTRL_SNAPSHOT_VERSION;
      snapshot.struct_size = sizeof(snapshot);
      snapshot.epoch = UINT64_MAX;
      snapshot.raised_flags = 0xa5;
      snapshot.expected_tr = (amu_matrix_view_t){tr, 256, 32, 0};
      for (unsigned s = 0; s < 2; ++s) {
        snapshot.scales[s].bytes = sf[s];
        snapshot.scales[s].capacity = 16;
      }
      for (unsigned fault = 0; fault < 10; ++fault) {
        amu_ctrl_snapshot_t bad = snapshot;
        if (fault == 0) bad.version = 1;
        if (fault == 1) --bad.struct_size;
        if (fault == 2) bad.reserved = 1;
        if (fault == 3) bad.expected_tr.capacity = 255;
        if (fault == 4) bad.expected_tr.bytes = NULL;
        if (fault == 5) bad.expected_tr.row_pitch = 31;
        if (fault == 6) bad.expected_tr.reserved = 1;
        if (fault == 7) bad.scales[side].capacity = 15;
        if (fault == 8) bad.scales[side].bytes = NULL;
        if (fault == 9) bad.expected_tr.row_pitch = 256;
        amu_ctrl_event_t compared = event;
        assert(check_amu_ctrl_snapshot(&compared, &bad) == -2);
        assert(amu_ctrl_queue_size() == 1 && tr[0] == 0xa5 && sf[side][0] == 0xa5);
        assert(bad.epoch == UINT64_MAX && bad.raised_flags == 0xa5);
      }
      amu_ctrl_event_t compared = event;
      assert(check_amu_ctrl_snapshot(&compared, &snapshot) == 0);
      const amu_scale_metadata_t *meta = &snapshot.scales[side].metadata;
      assert(snapshot.epoch == 0 && snapshot.raised_flags == (rejected ? 16u : 0u));
      assert(meta->valid == !rejected && meta->generation == 9 &&
        meta->rows == 8 && meta->blocks == 2 && meta->row_pitch == 2 &&
        meta->source_stride == 256 && meta->reg_id == reg && meta->side == side);
      assert(!snapshot.scales[!side].metadata.generation);
      assert(memcmp(tr, expected_tr, 256) == 0 && memcmp(sf[side], expected_sf, 16) == 0);
      assert(tr[256] == 0xa5 && sf[side][16] == 0xa5 && sf[!side][0] == 0xa5);
      assert(amu_ctrl_queue_empty());

      const size_t apply_reads = reads;
      amu_ctrl_event_t wrong = event;
      wrong.pc += 4;
      assert(apply_dynamic_snapshot_event(&wrong, &snapshot) != 0);
      for (unsigned row = 0; row < 8; ++row) {
        for (unsigned byte = 0; byte < TRENUM8; ++byte)
          assert(cpu.mtr[reg][row]._8[byte] == 0xe5);
      }
      assert(apply_dynamic_snapshot_event(&event, &snapshot) == 0);
      for (unsigned row = 0; row < 8; ++row) {
        assert(memcmp(cpu.mtr[reg][row]._8, expected_tr + row * 32, 32) == 0);
        assert(cpu.mtr[reg][row]._8[32] == 0xe5);
      }
      assert(apply_dynamic_snapshot_event(&event, &snapshot) != 0);
      assert(reads == apply_reads);

      // Both appended selectors participate in the original semantic compare.
      cpu.mscale_valid[index] = !rejected;
      cpu.mscale_rows[index] = 8;
      cpu.mscale_blocks[index] = 2;
      cpu.mscale_generation[index] = 9;
      cpu.mscale_source_stride[index] = 256;
      for (unsigned selector = 0; selector < 2; ++selector) {
        amu_ctrl_queue_push(event, rejected ? 16 : 0, &ref_result);
        compared = event;
        if (selector == 0) compared.isConvertF32 = 0;
        else compared.isDynamicMxfpQuant = 0;
        assert(check_amu_ctrl(&compared) == 1 && compared.isConvertF32 == 1 &&
          compared.isDynamicMxfpQuant == 1 && amu_ctrl_queue_empty());
      }
      ++checked;
    }
  }
  // Two same-PC dynamic generations stay distinct in delivery order. Each is
  // compared and applied once; a stale global TR is never the source.
  amu_ctrl_queue_reset();
  const unsigned reg = 2, index = reg;
  amu_ctrl_event_t event = {.valid = 1, .op = 1, .md = reg,
    .types2 = 1, .typed = 2, .base = 256, .stride = 256,
    .mtilem = 8, .mtilen = 64, .pc = 0x80001a00, .computeType = 10,
    .isConvertF32 = 1, .isDynamicMxfpQuant = 1};
  for (uint32_t generation = 9; generation <= 10; ++generation) {
    uint8_t expected_tr[256], tr[257], sf[2][17];
    memset(tr, 0xa5, sizeof(tr));
    memset(sf, 0xa5, sizeof(sf));
    for (unsigned i = 0; i < sizeof(expected_tr); ++i)
      expected_tr[i] = (uint8_t)(generation * 7 + i);
    for (unsigned row = 0; row < 8; ++row)
      for (unsigned block = 0; block < 2; ++block)
        cpu.mscale[index][row][block] = (uint8_t)(generation + row + block);
    cpu.mscale_valid[index] = 1;
    cpu.mscale_rows[index] = 8;
    cpu.mscale_blocks[index] = 2;
    cpu.mscale_generation[index] = generation;
    cpu.mscale_source_stride[index] = 256;
    cpu.mscale_mode[index] = 10;
    amu_matrix_view_t ref_result = {expected_tr, sizeof(expected_tr), 32, 0};
    amu_ctrl_queue_push(event, 0, &ref_result);
    amu_ctrl_snapshot_t snapshot = {0};
    snapshot.version = AMU_CTRL_SNAPSHOT_VERSION;
    snapshot.struct_size = sizeof(snapshot);
    snapshot.expected_tr = (amu_matrix_view_t){tr, 256, 32, 0};
    for (unsigned side = 0; side < 2; ++side) {
      snapshot.scales[side].bytes = sf[side];
      snapshot.scales[side].capacity = 16;
    }
    amu_ctrl_event_t compared = event;
    assert(check_amu_ctrl_snapshot(&compared, &snapshot) == 0);
    assert(snapshot.scales[0].metadata.generation == generation);
    assert(apply_dynamic_snapshot_event(&event, &snapshot) == 0);
    assert(memcmp(cpu.mtr[reg][0]._8, expected_tr, 32) == 0);
  }
  printf("PASS owned dynamic queue: %u A/B success/reject cases, immutable packed "
    "TR/SF/local flags across MINIT, v1/capacity/pitch/reserved refusals40, "
    "same-PC generations ordered\n", checked);
}

static void test_owned_lazy(void) {
  unsigned checked = 0;
  const unsigned ks[] = {1, 31, 32, 33, 63, 64, 65};
  const unsigned types[] = {0x16, 0x18, 0x17};
  const unsigned modes[] = {10, 7, 8};
  for (unsigned format = 0; format < 3; ++format) {
    for (unsigned ki = 0; ki < sizeof(ks) / sizeof(ks[0]); ++ki) {
      const unsigned k = ks[ki];
      const bool fp4 = format == 0;
      if (!fp4 && k > TRENUM8) continue;
      Decode d = scenario(types[format], 3, 5, k);
      d.pc = 0x80001000;
      prev_s = &d;
      amu_ctrl_queue_reset();
      load_scales(&d);
#ifdef CONFIG_DIFFTEST_AMU_CTRL
      // Consume the two automatic scale owners created by load_scales before
      // selecting the explicit MMA owner whose lazy execution is under test.
      static const unsigned lazy_rows[2] = {3, 5};
      const unsigned lazy_blocks = (k + 31) / 32;
      for (unsigned side = 0; side < 2; ++side) {
        const unsigned live_bytes = lazy_rows[side] * lazy_blocks;
        uint8_t auto_bytes[2][64];
        memset(auto_bytes, 0xa5, sizeof(auto_bytes));
        amu_ctrl_snapshot_t auto_snapshot = {0};
        auto_snapshot.version = AMU_CTRL_SNAPSHOT_VERSION;
        auto_snapshot.struct_size = sizeof(auto_snapshot);
        for (unsigned s = 0; s < 2; ++s) {
          auto_snapshot.scales[s].bytes = auto_bytes[s];
          auto_snapshot.scales[s].capacity = sizeof(auto_bytes[s]);
        }
        amu_ctrl_event_t auto_event = amu_ctrl_queue_front();
        assert(auto_event.op == 1 && auto_event.isScale &&
          auto_event.md == side && auto_event.types2 == (side == 0) &&
          auto_event.mtilem == lazy_rows[side] && auto_event.mtilen == lazy_blocks &&
          auto_event.computeType == modes[format]);
        amu_ctrl_event_t auto_compared = auto_event;
        assert(check_amu_ctrl_snapshot(&auto_compared, &auto_snapshot) == 0);
        const amu_scale_metadata_t *auto_meta = &auto_snapshot.scales[side].metadata;
        assert(auto_snapshot.epoch == 0 && auto_meta->valid &&
          auto_meta->generation == 1 && auto_meta->rows == lazy_rows[side] &&
          auto_meta->blocks == lazy_blocks && auto_meta->row_pitch == lazy_blocks &&
          auto_meta->reg_id == side && auto_meta->side == side &&
          auto_meta->source_stride == lazy_blocks + 3);
        for (unsigned byte = 0; byte < live_bytes; ++byte)
          assert(auto_bytes[side][byte] == 127);
        assert(auto_bytes[side][live_bytes] == 0xa5);
        assert(!auto_snapshot.scales[!side].metadata.valid);
      }
      assert(amu_ctrl_queue_empty());
#endif
      const unsigned code = fp4 ? 0x22 : format == 1 ? 0x38 : 0x3c;
      const unsigned row_bytes = fp4 ? (k + 1) / 2 : k;
      for (unsigned row = 0; row < 5; ++row) {
        if (row < 3) memset(cpu.mtr[0][row]._8, code, row_bytes);
        memset(cpu.mtr[1][row]._8, code, row_bytes);
        if (fp4 && (k & 1)) {
          if (row < 3) cpu.mtr[0][row]._8[k / 2] = 0xf2;
          cpu.mtr[1][row]._8[k / 2] = 0xf2; // Poison the inactive high nibble.
        }
      }
      const unsigned payload_type = fp4 ? 7 : format == 1 ? 4 : 0;
      amu_ctrl_queue_mma_emplace(4, 0, false, true, 0, 1, 3, 5, k,
        payload_type, payload_type, 2, modes[format]);
      amu_ctrl_event_t ctrl = amu_ctrl_queue_front();
      uint8_t scale_bytes[2][64];
      amu_ctrl_snapshot_t scales = {0};
      scales.version = AMU_CTRL_SNAPSHOT_VERSION;
      scales.struct_size = sizeof(scales);
      for (unsigned side = 0; side < 2; ++side) {
        scales.scales[side].bytes = scale_bytes[side];
        scales.scales[side].capacity = sizeof(scale_bytes[side]);
      }
      assert(check_amu_ctrl_snapshot(&ctrl, &scales) == 0);
      // Later live REF state must not supply the bound operation's scales.
      memset(cpu.mscale, 255, sizeof(cpu.mscale));
      uint8_t bytes[4][512];
      memset(bytes, 0xa5, sizeof(bytes));
      amu_mma_snapshot_t lazy = {0};
      lazy.version = AMU_MMA_SNAPSHOT_VERSION;
      lazy.struct_size = sizeof(lazy);
      lazy.scales = &scales;
      for (unsigned v = 0; v < 4; ++v) {
        lazy.matrices[v].bytes = bytes[v];
        lazy.matrices[v].capacity = sizeof(bytes[v]);
        lazy.matrices[v].row_pitch = v < 2 ? 80 : 32;
      }
      mfflags->val = 8; // Raised flags are not the preexisting sticky flags.
      mcsr->val = 0xa45;
      lazy.matrices[3].capacity = 2 * 32 + 5 * 4 - 1;
      assert(exec_amu_lazy_snapshot(&ctrl, &lazy) == -2);
      assert(bytes[0][0] == 0xa5 && bytes[3][0] == 0xa5 && macc_l32(4, 0, 0) == 0);
      lazy.matrices[3].capacity = sizeof(bytes[3]);
      lazy.version++;
      assert(exec_amu_lazy_snapshot(&ctrl, &lazy) == -2);
      lazy.version--;
      scales.scales[1].metadata.valid = 0;
      assert(exec_amu_lazy_snapshot(&ctrl, &lazy) == -2);
      scales.scales[1].metadata.valid = 1;
      assert(exec_amu_lazy_snapshot(&ctrl, &lazy) == 0);
      assert(lazy.raised_flags == 0 && mfflags->val == 8);
      assert(mcsr->val == 0xa45);
      for (unsigned row = 0; row < 3; ++row) {
        for (unsigned col = 0; col < 5; ++col) {
          uint32_t expected;
          memcpy(&expected, bytes[3] + row * 32 + col * 4, 4);
          assert(expected == dyadic_bits(k, 0, false));
          assert(macc_l32(4, row, col) == expected);
        }
      }
      assert(bytes[0][row_bytes] == 0xa5 && bytes[1][row_bytes] == 0xa5);
      assert(bytes[2][20] == 0xa5 && bytes[3][20] == 0xa5);
      if (format == 2 && k == 1) {
        cpu.mtr[0][0]._8[0] = 0x7d; // E5 sNaN: expected result and newly raised NV.
        assert(exec_amu_lazy_snapshot(&ctrl, &lazy) == 0);
        uint32_t expected;
        memcpy(&expected, bytes[3], 4);
        assert(expected == 0x7fc00000 && lazy.raised_flags == 16 && mfflags->val == 24);
        assert(mcsr->val == 0xac5); // Update alias, preserve all non-flags fields.
      }
      checked++;
    }
  }
  printf("PASS bounded independent REF lazy: %u format/K cases, odd tails, M3/N5, "
    "noncompact views, frozen scales, newly raised flags, capacity/ABI refusal\n", checked);
}
#endif

int main(int argc, char **argv) {
  if (argc == 3 && strcmp(argv[1], "--export-jsonl") == 0) {
    vector_file = fopen(argv[2], "wx"); // Never overwrite a frozen receipt.
    if (!vector_file) { perror(argv[2]); return 1; }
  } else if (argc != 1 && !(argc == 2 &&
      (strcmp(argv[1], "--transport") == 0 ||
       strcmp(argv[1], "--verify-exec") == 0 ||
       strcmp(argv[1], "--verify-lazy") == 0 ||
       strcmp(argv[1], "--transport-admitted") == 0 ||
       strcmp(argv[1], "--transport-bad-abi") == 0 ||
       strcmp(argv[1], "--convert-missing-scale") == 0))) {
    fprintf(stderr, "usage: %s [--export-jsonl NEW_PATH | --transport | "
      "--verify-exec | --verify-lazy | --transport-admitted | --transport-bad-abi | "
      "--convert-missing-scale]\n", argv[0]);
    return 2;
  }
  if (argc == 2 && strcmp(argv[1], "--convert-missing-scale") == 0) {
#if defined(CONFIG_RV_AME_MXFP_CONVERT_LOAD) && defined(CONFIG_FPU_SOFT) && \
    !defined(CONFIG_SHARE) && !defined(CONFIG_SHARE_REF) && \
    !defined(CONFIG_SHARE_CTRL) && !defined(CONFIG_DIFFTEST_AMU_CTRL)
    Decode d = scenario(0x16, 1, 1, 64);
    cpu.mode = MODE_M;
    cpu.gpr[11]._64 = 256;
    exec_mlacvt_f32_mx(&d);
    assert(!"missing SF must fail-stop, not report numerical NV/success");
    abort();
#else
    fprintf(stderr, "standalone converting loads are not enabled\n");
    return 2;
#endif
  }
  if (argc == 2 && strcmp(argv[1], "--transport") == 0) {
    require_mx_transport();
    return 0;
  }
#ifdef NEMU_MXFP_SHARED_TEST
  if (vector_file) {
    fprintf(stderr, "shared typed fixture does not export standalone JSONL\n");
    return 2;
  }
#endif
#ifdef NEMU_MXFP_QUEUE_TEST
  if (argc == 2 && (strcmp(argv[1], "--transport-admitted") == 0 ||
      strcmp(argv[1], "--transport-bad-abi") == 0)) {
    const bool valid = strcmp(argv[1], "--transport-admitted") == 0;
    const int status = amu_snapshot_negotiate(AMU_CTRL_SNAPSHOT_VERSION, sizeof(amu_ctrl_snapshot_t),
      sizeof(amu_mma_snapshot_t), sizeof(amu_ctrl_event_t), TLEN / 8, ALEN / 8 + !valid);
    assert(status == (valid ? 0 : -2));
    require_mx_transport();
    return 0;
  }
  if (argc == 2 && (strcmp(argv[1], "--verify-exec") == 0 ||
      strcmp(argv[1], "--verify-lazy") == 0)) {
    assert(amu_snapshot_negotiate(AMU_CTRL_SNAPSHOT_VERSION, sizeof(amu_ctrl_snapshot_t), sizeof(amu_mma_snapshot_t),
      sizeof(amu_ctrl_event_t), TLEN / 8, ALEN / 8) == 0);
    amu_ctrl_event_t unsupported = {.computeType = 7};
    if (strcmp(argv[1], "--verify-exec") == 0) exec_amu(&unsupported, NULL);
    else exec_amu_lazy(&unsupported, NULL, NULL, NULL, NULL);
    return 0; // An MX request must abort before touching missing snapshots.
  }
  Decode d = {0};
  d.pc = 0x80000000;
  prev_s = &d;
  amu_ctrl_queue_reset();
  amu_ctrl_queue_mma_emplace(4, 0, false, true, 0, 1, 3, 5, 33, 4, 4, 2, 7);
  amu_ctrl_event_t event = amu_ctrl_queue_front();
  assert(event.computeType == 7 && !event.isScale && event.pc == d.pc);
  amu_ctrl_event_t compared = event;
  assert(check_amu_ctrl(&compared) == 0);
  amu_ctrl_queue_push(event, 0, NULL);
  compared.computeType = 8;
  assert(check_amu_ctrl(&compared) == 1 && compared.computeType == 7);
  event.op = 1; event.isScale = 1; event.mtilem = 3; event.mtilen = 2;
  amu_ctrl_queue_push(event, 0, NULL);
  compared = event; compared.isScale = 0;
  assert(check_amu_ctrl(&compared) == 1 && compared.isScale == 1);
  amu_ctrl_queue_mls_emplace(1, false, false, false, true, 0x1000, 5, 3, 2, 0);
  event = amu_ctrl_queue_front();
  assert(!event.computeType && !event.isScale && event.types2 == 1);
  amu_ctrl_queue_pop();
  puts("PASS AMU append-only offsets, MX metadata, unscaled zero defaults");
  test_snapshot_admission();
  test_owned_scale_queue();
  test_owned_dynamic_queue();
  test_owned_lazy();
  test_active_load_compare();
#endif
#ifdef NEMU_MXFP_SHARED_TEST
  test_convert_load();
  test_dynamic_convert_load();
  puts("PASS NEMU MX shared typed regression");
  return 0;
#endif
  test_formats();
  test_supplied_encoding();
  test_convert_load();
  test_dynamic_convert_load();
  test_shapes();
  test_fp4_capacity();
  test_nonuniform_and_unscaled();
  test_exact_blocks();
  test_block_order_and_rounding();
  test_cute_raw_goldens();
  test_scale_ff_nodot();
  if (vector_file) {
    assert(vector_count == 35 + 9 + 22 + 66);
    if (fclose(vector_file) != 0) { perror(argv[2]); return 1; }
    printf("PASS JSONL export: %u checked vectors (35+9+22+66), %s\n",
      vector_count, argv[2]);
  }
  puts("PASS NEMU MX focused regression");
  return 0;
}
#endif
