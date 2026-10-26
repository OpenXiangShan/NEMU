# NEMU configuration files

NEMU uses Kconfig. Named targets compose machine, version and runtime layers,
feature fragments and target-specific settings. Public entry points remain
`make <name>_defconfig`; ordinary single-file defconfigs also work.

## Layout

```
configs/
├── riscv32/base.config              baseline for riscv32-pa
├── riscv64/
│   ├── base.config                  baseline for riscv64-pa
│   ├── xs/
│   │   ├── base.config             shared XS/KMH machine settings
│   │   ├── dut.config              standalone execution and devices
│   │   ├── ref.config              shared REF runtime and device selection
│   │   ├── kmh/
│   │   │   ├── v2/
│   │   │   │   ├── base.config     V2 ISA compatibility
│   │   │   │   └── ref.config     V2 REF differences
│   │   │   └── v3/
│   │   │       ├── base.config     current mainline/V3 ISA compatibility
│   │   │       └── ref.config     V3 REF differences
│   │   └── {ahead,cpt,fpga,novga}.config   separate complete machine profiles
│   ├── boom/                      base + ref policy + FPGA UART layer
│   ├── southlake/                 base + dut/ref/fpga runtime layers
│   ├── gem5/                      base + ref + ci-normal/ci-h workload layers
│   └── {nutshell,nanhu,yanqihu}.config    complete machine profiles
├── fragments/
│   ├── difftest/                   ref-share, spike, multicore, fastref
│   ├── checkpoint/                 cpt (ZSTD), flash
│   ├── debug/                      debug
│   ├── toolchain/                  clang
│   ├── mem/                        dedup
│   └── ext/                        ame-matrix, mpt, bitmap, custom-tensor
└── <target>_defconfig               manifest or ordinary defconfig
```

A `base.config` owns shared machine settings: ISA features, the memory map,
reset, PMP/PMA sizing and common build options. It is more than an ISA-only
baseline. `dut`, `ref` and `fpga` layers add runtime-specific devices and
behavior. `dut` names the standalone execution mode; `ref` names the shared
reference mode. Within each factored machine family, the base and runtime
layers define disjoint sets of symbols. They are deltas to the base, not
complete configurations to load on their own. Other machine profiles remain
complete where there is no useful common base to factor out.

The directory gives a base its scope: `xs/base.config` is common to V2/V3,
while `kmh/v2/base.config` and `kmh/v3/base.config` contain each version's
settings shared by DUT and REF. A base need not contain only ISA settings;
there is no separate `isa.config` layer at the same scope.

KMH V2 and V3 have separate directories and use the same XS base and runtime
layers. `v2/base.config` and `v3/base.config` pin seven differences: CBO/PBMT
checks, MNRET state handling, trigger `tdata3`, Smcntrpmf, extra indirect CSR
registers, `sstatus.MS` and Zabha. `xs/ref.config` contains only REF settings
common to both versions. `v2/ref.config` and `v3/ref.config` each own the
version's Xvexp2/BF16 support, Smcdeleg and `GEILEN`, without redefining symbols
from the shared REF layer.
This keeps version/runtime combinations explicit without duplicating whole
machine profiles. `v3/base.config` preserves existing non-kmhv2 XS behavior; it
does not restore the retired `CUSTOM_CSR_KMHV3` implementation.

GEM5's `ref` layer supplies the shared-reference ABI/runtime settings.
`ci-normal` adds the ISA and trigger settings for the existing normal CI
profiles. `ci-h` supplies shared trap/translation settings for the H and
multicore CI targets; H is already enabled by Kconfig defaults. These files
are named for their test policy, not for a general CI or ISA enable switch.

BOOM uses `boom/base.config` for its machine properties, including 2 GiB
DRAM, 16 PMP entries, Sv39 and the Rocket/BOOM `medeleg` WARL mask. Its REF
targets apply `fragments/difftest/ref-share.config` followed by
`boom/ref.config` for BOOM comparison policy. The optional `boom/fpga.config`
adds the board UART16550 at `0x310b0000` with register shift 2. Fast targets
append the shared `fragments/difftest/fastref.config`; no BOOM-specific fast
interpreter implementation is needed.

`riscv64-boom-ref` and `riscv64-boom-fastref` select the normal and fast
references without the board UART. `riscv64-boom-fpga-ref` and
`riscv64-boom-fpga-fastref` add that UART. Each name takes the usual
`_defconfig` suffix. The existing `riscv64-boom-fast-ref_defconfig` remains
a compatibility entry for the FPGA fast reference, preserving its behavior.

Feature fragments carry reusable capabilities. Machine/version/runtime
layers may intentionally override earlier values; a final target holds its
remaining local exceptions. No file implicitly inherits or includes another:
the manifest lists every input in application order. The top-level
`riscv32/base.config` and `riscv64/base.config` serve only the `*-pa` targets;
they are not implicit parents of machine bases.

## Building and composing

```sh
make riscv64-xs-ref_defconfig
make -j"$(nproc)"
```

For example, the V2 REF target lists:

```
# layer: riscv64/xs/base.config
# layer: riscv64/xs/kmh/v2/base.config
# layer: riscv64/xs/ref.config
# layer: riscv64/xs/kmh/v2/ref.config
```

The mainline REF uses `base`, `kmh/v3/base`, shared `ref` and `kmh/v3/ref`.
Optional multicore, debug, fastref or extension fragments follow;
target-specific assignments go last.
Special targets such as XS checkpoint and FPGA use their own machine profiles.
All existing public target names are retained.

`merge_target.sh` validates the layer list, `merge_config.sh` merges its inputs,
then Kconfig resolves dependencies, choices and defaults. Feed manifests
through make or the merger: `conf` itself does not understand `# layer:`.
Make passes the merged input to `conf --defconfig` to write `.config`, then
runs `conf --syncconfig` to update the generated configuration headers.

```sh
# Show the layer list or compose a target with override diagnostics.
scripts/merge_target.sh --list configs/riscv64-xs-ref_defconfig
scripts/merge_target.sh -r -o /tmp/nemu.config configs/riscv64-xs-ref_defconfig

# The lower-level merger also accepts an explicit list.
scripts/merge_config.sh -r -o /tmp/nemu.config \
    configs/riscv64/xs/base.config configs/riscv64/xs/kmh/v3/base.config \
    configs/riscv64/xs/ref.config configs/riscv64/xs/kmh/v3/ref.config \
    configs/fragments/ext/mpt.config
```

## Override rules

1. Layers are applied in listed order. The last explicit definition of a symbol
   wins and keeps its position. Listing a file twice reapplies it.
2. The manifest's own settings are always merged last, even if a layer
   directive appears after a setting. Keep directives first for readability.
3. `# CONFIG_X is not set` means `X=n`; it is a configuration directive.
   A missing symbol follows Kconfig defaults and dependencies instead.
4. Choice members are distinct symbols. When switching members, disable the
   previous one explicitly. CI rejects Kconfig choice diagnostics.
5. Textual precedence does not bypass dependencies or `select`. The generated
   `.config` is the effective configuration.

`-r` distinguishes changed values (`override`, with both source files) from
repeated equal values (`redundant`). Changing an omitted symbol can change its
Kconfig default without being a textual override.

Keep common settings in shared layers and version differences in the matching
version layer. Reserve final-target overrides for local exceptions. Feature
fragments should not silently replace another fragment's policy or carry board
addresses and sizes. Forbidding all overrides would either duplicate complete
profiles or repeat shared feature settings in every target.

Equal assignments in shared fragments may be needed by other consumers. Delete
them only after checking all users. Unpinned settings follow Kconfig defaults:
record the repository revision and effective `.config` when reproducing a
build. A target name alone does not pin a source version.

## Checking and maintaining

`make config-check` composes every named target without replacing the working
`.config`. It runs the merger, manifest and checker regression suites, checks
Kconfig diagnostics, target ISA, determinism and unknown symbols, and reports
unused layers and override/redundancy counts.

It also compares merged requests with the effective `.config`:

* Requested `y`/`m` must retain that value.
* Any present effective value must match its request, including an explicit
  `n` that Kconfig might otherwise force to `y`. Missing `n` counts as disabled.
* Missing int/hex/string parameters are reported as `dormant`, not errors: a
  machine's flash size becomes inactive after a target disables flash. This
  report does not prove the intended device is enabled; review it on changes.

When disabling a parent capability, explicitly disable inherited boolean
children too. Do not use `# CONFIG_X is not set` to delete scalar parameters;
that syntax disables bool/tristate values only.

To add a target, select its layers, create the manifest and local settings,
then configure, build and run `make config-check`. Use `make savedefconfig` to
save a complete minimal configuration when useful. It minimizes against Kconfig
defaults, not against a selected base, so it is not a fragment minimizer.
`make menuconfig` edits the current configuration; `make savedefconfig` writes
`configs/defconfig`, and `make defconfig` loads it.

Inputs under `configs/` are tracked. `.config` (or `$KCONFIG_CONFIG`) is a build
output. The tracked `riscv64-xs-ref-perf_defconfig` replaces the old perf config
generator; CI uses an older baseline's own generator only when that baseline
predates the tracked target. `config-check` replaces config bumping and does
not rewrite manifests as complete configs.
