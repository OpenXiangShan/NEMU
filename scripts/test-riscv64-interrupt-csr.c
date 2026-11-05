// SPDX-License-Identifier: MulanPSL-2.0
// Link against a REF build made with the same generated configuration.
#include <stdio.h>
#include <isa.h>
#include "../src/isa/riscv64/local-include/csr.h"
#include "../src/isa/riscv64/local-include/intr.h"

#if !defined(CONFIG_SHARE) || !defined(CONFIG_RVH) || !defined(CONFIG_RV_SSTC)
#error "Build an RVH+Sstc REF configuration before running this test"
#endif

void riscv64_priv_csrrw(rtlreg_t *, word_t, word_t, word_t);
void riscv64_priv_csrrs(rtlreg_t *, word_t, word_t, word_t);
void riscv64_priv_csrrc(rtlreg_t *, word_t, word_t, word_t);
void csr_writeback(void);

static unsigned checks, failures;

static void check(const char *name, word_t actual, word_t expected) {
  ++checks;
  if (actual != expected) {
    ++failures;
    printf("FAIL %s: actual=0x%lx expected=0x%lx\n", name, actual, expected);
  }
}

static void reset(void) {
  memset(csr_array, 0, sizeof(csr_array));
  memset(&cpu.non_reg_interrupt_pending, 0, sizeof(cpu.non_reg_interrupt_pending));
  cpu.mode = MODE_M;
  cpu.v = 0;
  cpu.hideleg_reg = 0;
  menvcfg->stce = 1;
}

static void write_csr(word_t csr, word_t value) {
  rtlreg_t ignored;
  riscv64_priv_csrrw(&ignored, value, csr, 0);
}

static void test_alias_write(word_t csr, bool stce) {
  reset();
  menvcfg->stce = stce;
  cpu.non_reg_interrupt_pending.platform_irp_stip = 1;
  cpu.non_reg_interrupt_pending.platform_irp_mtip = 1;
  cpu.non_reg_interrupt_pending.platform_irp_vstip = 1;
  write_csr(csr, 0);
  check("alias write must not store timer inputs",
        mip->val & (MIP_STIP | MIP_MTIP | MIP_VSTIP), 0);
  memset(&cpu.non_reg_interrupt_pending, 0, sizeof(cpu.non_reg_interrupt_pending));
  check("timer inputs deassert", get_mip() & (MIP_STIP | MIP_MTIP | MIP_VSTIP), 0);
#ifndef CONFIG_RV_AIA
  reset();
  cpu.non_reg_interrupt_pending.platform_irp_seip = 1;
  write_csr(csr, 0);
  check("alias write must not store external SEIP", mip->val & MIP_SEIP, 0);
  cpu.non_reg_interrupt_pending.platform_irp_seip = 0;
  check("external SEIP deasserts", get_mip() & MIP_SEIP, 0);
#endif
}

static void test_alias_writable_bits(void) {
#ifdef CONFIG_RV_AIA
  reset();
  write_csr(CSR_MVIP, MIP_SSIP);
  check("MVIP writes aliased software SSIP", mip->ssip, 1);
  reset();
  mvien->ssie = 1;
  write_csr(CSR_MVIP, MIP_SSIP);
  check("MVIP writes independent software SSIP", mvip->ssip, 1);
  reset();
  menvcfg->stce = 0;
  write_csr(CSR_MVIP, MIP_STIP);
  check("MVIP writes software STIP when STCE=0", mip->stip, 1);
  write_csr(CSR_MVIP, 0);
  check("MVIP clears software STIP when STCE=0", mip->stip, 0);
#endif
  reset();
  mideleg->val = MIP_VSSIP;
  cpu.hideleg_reg = MIP_VSSIP;
  write_csr(CSR_VSIP, MIP_SSIP);
  check("VSIP writes delegated virtual SSIP", hvip->vssip, 1);
  write_csr(CSR_VSIP, 0);
  check("VSIP clears delegated virtual SSIP", hvip->vssip, 0);
#if defined(CONFIG_RV_SSCOFPMF) && defined(CONFIG_RV_SHLCOFIDELEG)
  reset();
  mideleg->val = MIP_LCOFIP;
  cpu.hideleg_reg = MIP_LCOFIP;
  write_csr(CSR_VSIP, MIP_LCOFIP);
  check("VSIP writes delegated LCOFIP", mip->lcofip, 1);
  write_csr(CSR_VSIP, 0);
  check("VSIP clears delegated LCOFIP", mip->lcofip, 0);
#endif
}

static word_t software_seip(void) {
#ifdef CONFIG_RV_AIA
  return mvip->val & MIP_SEIP;
#else
  return mip->val & MIP_SEIP;
#endif
}

static void test_seip_rmw(void) {
  const word_t masks[] = {0, MIP_SSIP, MIP_SEIP};
  for (unsigned op = 0; op < 2; ++op) {
    for (unsigned software = 0; software < 2; ++software) {
      for (unsigned external = 0; external < 2; ++external) {
        for (unsigned i = 0; i < sizeof(masks) / sizeof(masks[0]); ++i) {
          reset();
          write_csr(CSR_MIP, software ? MIP_SEIP : 0);
          cpu.non_reg_interrupt_pending.platform_irp_seip = external;
          rtlreg_t old;
          if (op == 0) {
            riscv64_priv_csrrs(&old, masks[i], CSR_MIP, 1);
          } else {
            riscv64_priv_csrrc(&old, masks[i], CSR_MIP, 1);
          }
          word_t before = software ? MIP_SEIP : 0;
          word_t expected = (op == 0 ? before | masks[i] : before & ~masks[i]) & MIP_SEIP;
          check("RMW rd includes external SEIP", old & MIP_SEIP,
                (software || external) ? MIP_SEIP : 0);
          check("RMW writes only software SEIP", software_seip(), expected);
          cpu.non_reg_interrupt_pending.platform_irp_seip = 0;
          check("RMW SEIP after external deassertion", get_mip() & MIP_SEIP, expected);
          check("RMW other writable bits", mip->val & MIP_SSIP,
                op == 0 ? masks[i] & MIP_SSIP : 0);
        }
      }
    }
  }
  reset();
  cpu.non_reg_interrupt_pending.platform_irp_seip = 1;
  rtlreg_t old;
  riscv64_priv_csrrs(&old, 0, CSR_MIP, 0);
  check("read-only CSRRS returns external SEIP", old & MIP_SEIP, MIP_SEIP);
  check("read-only CSRRS leaves software SEIP", software_seip(), 0);
  riscv64_priv_csrrc(&old, 0, CSR_MIP, 0);
  check("read-only CSRRC returns external SEIP", old & MIP_SEIP, MIP_SEIP);
  check("read-only CSRRC leaves software SEIP", software_seip(), 0);
  riscv64_priv_csrrw(&old, MIP_SSIP, CSR_MIP, 1);
  check("CSRRW returns external SEIP", old & MIP_SEIP, MIP_SEIP);
  check("CSRRW writes requested software SEIP", software_seip(), 0);
  cpu.non_reg_interrupt_pending.platform_irp_seip = 0;
  check("CSRRW external SEIP deasserts", get_mip() & MIP_SEIP, 0);
#ifdef CONFIG_RV_AIA
  reset();
  cpu.non_reg_interrupt_pending.from_aia_seip = 1;
  riscv64_priv_csrrs(&old, MIP_SSIP, CSR_MIP, 1);
  check("CSRRS returns AIA controller SEIP", old & MIP_SEIP, MIP_SEIP);
  check("CSRRS must not store AIA controller SEIP", software_seip(), 0);
  reset();
  mvien->seie = 1;
  mvip->seip = 1;
  riscv64_priv_csrrc(&old, MIP_SEIP, CSR_MIP, 1);
  check("MVIEN.SEIE=1 excludes virtual SEIP from mip", old & MIP_SEIP, 0);
  check("MVIEN.SEIE=1 preserves independent mvip.SEIP", software_seip(), MIP_SEIP);
#endif
}

static void test_stip_writeback(void) {
  for (unsigned stce = 0; stce < 2; ++stce) {
    for (unsigned software = 0; software < 2; ++software) {
      for (unsigned signal = 0; signal < 2; ++signal) {
        reset();
        menvcfg->stce = stce;
        mip->stip = software;
        mip->ssip = 1;
        cpu.non_reg_interrupt_pending.platform_irp_stip = signal;
        csr_prepare();
        csr_writeback();
        check("writeback preserves software STIP", mip->stip, software);
        check("writeback preserves other writable bits", mip->ssip, 1);
        cpu.non_reg_interrupt_pending.platform_irp_stip = 0;
        menvcfg->stce = 0;
        check("STCE change reveals original software STIP", get_mip() & MIP_STIP,
              software ? MIP_STIP : 0);
      }
    }
  }
}

int main(void) {
  init_isa();
  test_alias_write(CSR_SIP, true);
#if defined(CONFIG_RV_AIA) || defined(CONFIG_RV_SSCOFPMF)
  test_alias_write(CSR_VSIP, true);
#endif
#ifdef CONFIG_RV_AIA
  test_alias_write(CSR_MVIP, true);
  test_alias_write(CSR_MVIP, false);
#endif
  test_alias_writable_bits();
  test_seip_rmw();
  test_stip_writeback();
  printf("Interrupt CSR regression: %u checks, %u failures (AIA=%d)\n",
         checks, failures, ISDEF(CONFIG_RV_AIA));
  return failures != 0;
}
