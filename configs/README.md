# NEMU configuration files

NEMU uses Kconfig. Named configurations consist of a self-contained machine
profile, optional feature fragments and target-specific settings. The public
entry points remain `make <name>_defconfig`.

## Layout

```
configs/
├── riscv32/isa.config              baseline for riscv32-pa
├── riscv64/
│   ├── isa.config                  baseline for riscv64-pa
│   ├── xs/                        sim, ref, fpga, ahead, novga, cpt,
│   │                              kmhv2 and kmhv2-ref profiles
│   ├── southlake/                 sim, ref and fpga profiles
│   ├── gem5/                      ref and ci profiles
│   └── {nutshell,nanhu,yanqihu}.config
├── fragments/
│   ├── difftest/                  ref-share, spike, multicore, fastref
│   ├── checkpoint/                cpt (ZSTD), flash
│   ├── debug/                     debug
│   ├── toolchain/                 clang
│   ├── mem/                       dedup
│   └── ext/                       ame-matrix, mpt, bitmap, custom-tensor
└── <target>_defconfig              manifest or ordinary defconfig
```

A machine profile is a complete usable configuration expressed as a delta
from the Kconfig defaults, like a Linux board defconfig. Profiles are flat:
none includes or inherits another profile. Machine properties such as memory
map, reset path, PMP/PMA sizing and extension compatibility belong here.
The `isa.config` baselines are used by the `*-pa` targets, not automatically
inherited by every profile.

A fragment describes a shared capability, such as multicore difftest,
checkpoint compression or an ISA extension. Machine-specific parameters stay
in the profile or final target. Keep features that have only one user in that
target unless a separate fragment makes their purpose substantially clearer.

## Building and composing

```sh
make riscv64-xs-ref_defconfig
make -j"$(nproc)"
```

A manifest lists paths relative to `configs/`, followed by its own settings:

```
# layer: riscv64/xs/ref.config
# layer: fragments/ext/ame-matrix.config
# layer: fragments/difftest/multicore.config

CONFIG_DIFFTEST_AMU_CTRL=y
# CONFIG_RV_SMCNTRPMF is not set
# CONFIG_RV_SMCDELEG is not set
```

`merge_target.sh` parses the list, `merge_config.sh` merges the inputs, then
Kconfig evaluates dependencies, choices and defaults. A file without layer
directives is an ordinary defconfig and follows the same make entry point.
Feed manifests through make or the merger, not directly to `conf`: Kconfig
itself does not understand `# layer:`.

```sh
# Show the manifest's layers, in application order.
scripts/merge_target.sh --list configs/riscv64-xs-ref_defconfig

# Compose and report every changed or repeated assignment.
scripts/merge_target.sh -r -o /tmp/nemu.config \
    configs/riscv64-xs-ref_defconfig

# The lower-level merger also accepts an explicit list.
scripts/merge_config.sh -r -o /tmp/nemu.config \
    configs/riscv64/xs/ref.config configs/fragments/ext/mpt.config
```

## Override rules

1. Layers are applied in the order listed. For each symbol, the last explicit
   definition wins and keeps the position of that last definition. Listing a
   file twice reapplies it.
2. The manifest's own settings are always merged last, even if a layer
   directive appears after a setting in the file. Keep directives first for
   readability.
3. `# CONFIG_X is not set` means `X=n`; it is a configuration directive.
   A missing symbol instead follows Kconfig defaults or dependencies.
4. Choice members are distinct symbols: switching one on does not textually
   remove another. Disable the previous member explicitly. CI rejects Kconfig
   choice diagnostics.
5. Textual precedence does not bypass Kconfig dependencies or `select`. The
   generated `.config` is the effective configuration.

The merger permits intentional overrides. `-r` distinguishes changed values
(`override`, with both source files) from repeated equal values (`redundant`).
Changing a symbol omitted by a profile is not a textual override, even if the
fragment changes its Kconfig default.

Prefer final-target overrides for machine/version exceptions. A reusable
fragment may intentionally change a capability, but should not quietly replace
another fragment's policy or carry board addresses and memory sizes. There is
no global ban on overrides: forbidding them would force duplicated profiles or
move shared feature policy into every target. Independent version profiles
remain useful when their standalone machine behavior differs.

Equal definitions in shared fragments may be needed by other users. Delete
them only after checking every consumer. New unpinned symbols follow Kconfig
defaults; pin behavior that must remain stable, and record the repository
revision plus generated `.config` when reproducing a build. A target name alone
does not pin a source version.

## Checking and maintaining

`make config-check` composes all named targets without replacing the working
`.config`. It runs the merger, manifest and checker regression suites, then
checks Kconfig diagnostics, target ISA, determinism and unknown symbols. It
reports unused layers and override/redundancy counts.

The checker also compares merged requests with the effective `.config`:

* Requested `y`/`m` must remain enabled with that value.
* Any present effective value must match its request, including an explicit
  `n` that Kconfig might otherwise force to `y`. Missing `n` counts as disabled.
* Missing scalar parameters (int/hex/string) are reported as `dormant`, not
  errors. For example, a profile's flash size is inactive after a target
  disables flash. This report does not prove that an intended device was
  enabled; review it when adding or changing a target.

When a target disables a parent capability, explicitly disable inherited
boolean children too. Do not use `# CONFIG_X is not set` to delete a scalar:
Kconfig only gives that syntax a disabling meaning for bool/tristate symbols.

To add a target, select a profile and fragments, add the manifest and its
unique settings, then run `make <name>_defconfig`, build, and run
`make config-check`. Derive a new independent profile with `make savedefconfig`
when appropriate; do not add profile inheritance.

`make menuconfig` edits the current configuration. `make savedefconfig` saves
a minimal ordinary file to `configs/defconfig`; `make defconfig` loads it.
`savedefconfig` minimizes against Kconfig defaults, not against a chosen base
profile, so it is not a fragment minimizer.

Input files under `configs/` are tracked; `.config` (or `$KCONFIG_CONFIG`) is
the build output. `riscv64-xs-ref-perf_defconfig` is a tracked target. The old
config-bumping and perf-generation scripts are removed; CI uses `config-check`
and uses the baseline revision's own generator only for older baselines.
