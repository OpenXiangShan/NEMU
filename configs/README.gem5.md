# GEM5 CI reference configurations

These configurations share one NEMU source revision, not one shared library.
They supplement the existing `riscv64-gem5-ref_defconfig` and
`riscv64-gem5-multicore-ref_defconfig`, which remain unchanged.

| Configuration suffix | Use | Memory | Vector agnostic |
| --- | --- | --- | --- |
| `ci-normal-ref` | Scalar SPEC, RVV tests, solver | Private mmap | Yes |
| `ci-normal-dedup-ref` | Single-core checkpoint performance CI | External COW | Yes |
| `ci-h-ref` | H tests | Private mmap | No |
| `ci-multi-ref` | SMT and multicore difftest | External COW | Yes |

For example, after initializing the normal NEMU build dependencies:

```sh
export NEMU_HOME="$PWD"
make riscv64-gem5-ci-normal-ref_defconfig
make -j8
```

Build each variant in a separate checkout/build tree and retain its `.config`
alongside the resulting library. Do not overwrite a library used by older GEM5.
These are native `savedefconfig` outputs, so omitted settings follow Kconfig
defaults. At the introduction revision all four expand to RVV, RVH, VCSR and
FCSR comparison enabled, trigger-register comparison disabled, Sv48,
`TVAL_EX_II=y` and `GUIDED_TVAL=y`. The exported `DIFFTEST_REG_SIZE` is 1376
bytes. Consumers must validate the register layout before calling `regcpy`.

The normal variants include the extensions needed by current GEM5 scalar CI,
including Zacas for GCC16 SPEC17. This is not a claim of complete RVA23
compliance; in particular Zabha and Smcdeleg are not enabled. H and multicore
intentionally do not inherit these extra extensions. Enabling SSTC for the H
suite, for example, changes the expected `stimecmp` illegal-instruction behavior.

The dedup variants require GEM5 to supply backing memory before execution.
Single-core users need the matching `--enable-mem-dedup` option from
[GEM5 PR #1105](https://github.com/OpenXiangShan/GEM5/pull/1105).
Do not select them as drop-in replacements for private-memory references.

## Validation

On NEMU `d30fff1ece9e0480146caf504660a0239b994eef`, these configurations
expand identically to the four configurations built and tested for GEM5 PR
#1105. All four libraries export the 1376-byte register layout. With the GEM5
FCSR/ABI changes and widening-reduction fflags fix, validation included:

- 878/878 vector microtests with the normal reference.
- Focused SPEC06 (GCC12 and GCC16), SPEC17 GCC16 and SPEC26 checkpoint tests.
- H checkpoint/restorer tests, SMT CoreMark and mcf, and a true two-core CHI test.
- Paired single-core private/dedup tests with identical simulated instruction
  counts, ticks, cycles and IPC.

These are targeted tests, not full SPEC/H/SMT regression coverage. See the
companion GEM5 PR for binary identities, dependency pins and validation details.
