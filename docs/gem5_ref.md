# GEM5 CI references

Build each variant from the same NEMU revision in a separate checkout:

```sh
export NEMU_HOME="$PWD"
make riscv64-gem5-ci-normal-ref_defconfig
make -j8
```

| Defconfig suffix | Use | Memory | RVV agnostic |
| --- | --- | --- | --- |
| `ci-normal-ref` | Scalar SPEC, RVV, solver | Private mmap | Yes |
| `ci-normal-dedup-ref` | Single-core checkpoint CI | External COW | Yes |
| `ci-h-ref` | H tests | Private mmap | No |
| `ci-multi-ref` | SMT and multicore | External COW | Yes |

All four use Sv48 and the 1376-byte RVV/RVH/VCSR/FCSR register layout,
without trigger-register comparison. GEM5 must check `DIFFTEST_REG_SIZE`
before `regcpy`. Keep each library with its `.config`; do not overwrite
references used by older GEM5 binaries.

Normal variants include Zacas for GCC16 SPEC17, but do not provide complete
RVA23 support. H/multi intentionally omit the scalar extension additions,
including SSTC, to preserve their expected CSR behavior.

Dedup variants require GEM5-provided backing memory. Single-core use requires
`--enable-mem-dedup`; private-memory references must not use that option.
See [GEM5 PR #1105](https://github.com/OpenXiangShan/GEM5/pull/1105) for the
consumer changes and validation results. Existing GEM5 defconfigs are unchanged.
