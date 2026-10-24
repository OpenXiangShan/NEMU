# NEMU configuration files

NEMU is configured with Kconfig (`Kconfig`, `.config`, `make menuconfig`).
This directory holds the *named* configurations: one entry point per supported
machine or variant, plus the layers those entry points are built from.

The configurations used to be stored as 50+ complete `.config` dumps.  A
setting shared by several machines was written down in every file and a change
to the common platform had to be repeated everywhere.  They are now split into
*self-contained machine profiles* and small *feature fragments* that are
composed on demand.  This follows the Linux kernel's model: board
`defconfig`s are complete configurations, fragments are overlays applied on top
of them, and `scripts/kconfig/merge_config.sh` merges an explicit list of
files (later definitions win).

## Layout

```
configs/
├── riscv64/
│   ├── isa.config                  ISA baseline (used by the *-pa targets)
│   ├── xs/                         XiangShan machines
│   │   ├── sim.config              simulation machine     -> riscv64-xs
│   │   ├── ref.config              difftest reference     -> riscv64-xs-ref
│   │   ├── fpga.config             FPGA machine           -> riscv64-xs-fpga
│   │   ├── ahead.config            "ahead" reference      -> riscv64-xs-ahead-ref
│   │   └── novga.config            lean machine, no VGA   -> riscv64-xs-novga
│   ├── southlake/
│   │   ├── sim.config              SouthLake machine      -> riscv64-xs-southlake
│   │   ├── ref.config              difftest reference     -> riscv64-xs-southlake-ref
│   │   └── fpga.config             FPGA machine           -> riscv64-xs-southlake-fpga
│   ├── nutshell.config             NutShell machine       -> riscv64-nutshell
│   ├── nanhu.config                NanHu machine          -> riscv64-nanhu
│   ├── yanqihu.config              YanQiHu machine        -> riscv64-yanqihu
│   └── gem5/
│       ├── ref.config              GEM5 reference         -> riscv64-gem5-ref
│       └── ci.config               GEM5 CI machine        -> riscv64-gem5-ci-*-ref
├── riscv32/
│   └── isa.config                  ISA baseline for riscv32-pa
├── fragments/                      overlays shared by several machines
│   ├── difftest/                   ref-share, spike, multicore, ctrl
│   ├── checkpoint/                 cpt, libcheckpoint, flash
│   ├── debug/debug.config
│   ├── toolchain/clang.config
│   ├── mem/dedup.config
│   └── ext/                        ame-matrix, mpt, bitmap, zicfilp,
│                                   custom-tensor, kunminghu-v3
└── <target>_defconfig              composition manifests (53)
```

A **machine profile** is a complete configuration for one machine: applying
`conf --defconfig=configs/riscv64/xs/ref.config` on a clean tree yields a
usable `.config` (everything else comes from the Kconfig defaults).  Profiles
are flat and self-contained; they do not include or inherit from each other.
The only shared baseline is `configs/<isa>/isa.config`, which the `*-pa`
targets use directly.

A **fragment** is an overlay for a build/run mode or an ISA extension.  It is
applied after a profile and may override profile values (for example
`checkpoint/cpt` turns on ZSTD images).  A fragment only contains the settings
that *all* targets using it agree on; anything machine-specific belongs in a
profile or in the target's own overrides.

## Composition

`make <target>_defconfig` reads the target's layer list and merges it, then
installs the result with Kconfig exactly like a full `.config` before:

```sh
make riscv64-xs-ref_defconfig
make -j"$(nproc)"
```

The merge itself is a standalone script that takes an explicit file list, like
the kernel's `merge_config.sh`:

```sh
scripts/merge_config.sh -o /tmp/nemu.config \
    configs/riscv64/xs/ref.config configs/fragments/ext/mpt.config

# -r reports redefined and redundant symbols
scripts/merge_config.sh -r -o /tmp/nemu.config \
    configs/riscv64/xs/sim.config configs/fragments/checkpoint/cpt.config
```

Rules:

* The layer list is explicit and ordered.  For each `CONFIG_*` symbol the last
  definition wins and it keeps the position of that last definition, so Kconfig
  choice precedence is preserved.
* A layer that switches to another member of a Kconfig choice should also
  disable the previous member (`# CONFIG_X is not set`); `make config-check`
  fails if Kconfig reports an override warning for a target.
* Including the same file twice re-applies it.
* CRLF and trailing blanks on `# CONFIG_X is not set` are normalized; malformed
  configuration-looking lines are errors instead of being dropped silently.
* Symbols that are not mentioned keep their Kconfig default.  The layers are
  therefore *sparse*: pin a machine-defining capability in the profile if it
  must not follow Kconfig default changes.
* The merged result is ordinary `.config` syntax.

## Target files

A target is a small **composition manifest**, not a `.config`.  It lists its
layers with `# layer:` lines (paths relative to `configs/`) and keeps the
settings that are unique to it:

```
# configs/riscv64-matrix-xs-dual-ref_defconfig
#
# layer: riscv64/xs/ref.config
# layer: fragments/ext/ame-matrix.config
# layer: fragments/difftest/multicore.config
#
# Target-specific overrides.
CONFIG_DIFFTEST_AMU_CTRL=y
```

Feeding a manifest directly to `conf` would only see its own overrides, so
always use `make <target>_defconfig` (or `scripts/merge_config.sh` with the
layer list).

## Overlap and overrides

Layered configurations overlap by design: a fragment may change a value that a
profile set.  That is the same model as the kernel's fragments and it is not an
error, but it should be visible:

* `scripts/merge_config.sh -r` reports `override: CONFIG_X old -> new` and
  `redundant: CONFIG_X` lines.
* `make config-check` counts both per target and prints the totals, and points
  out targets with redundant definitions.
* A `redundant` entry (the later layer writes the same value again) is dead
  weight and a cleanup candidate.  A large number of overrides in a fragment
  usually means that machine settings ended up in a fragment; move them into
  the profile or the target.

## Where does a setting belong?

* Shared by every machine of an ISA (ISA choice, reset offset):
  `configs/<isa>/isa.config` (the `*-pa` targets) or the machine profiles.
* Part of one machine (memory map, devices, extension set, core model):
  the machine profile.
* A build/run mode or an ISA extension several machines can enable: a fragment
  under `configs/fragments/`.
* Only one target needs it: the `# Target-specific overrides.` section of that
  target.

## Adding a target or a profile

1. Pick the machine profile (or add one, derived from the canonical target
   with `make savedefconfig`) and the fragments you need.
2. Create `configs/<name>_defconfig` with the `# layer:` list plus any
   target-specific settings.
3. `make <name>_defconfig && make -j` and check the result.
4. `make config-check` to verify the whole tree.

## Checking the tree

`make config-check` (= `scripts/check_configs.sh`) is part of CI and verifies
that

* the merger passes its own regression tests (`scripts/merge_config_test.sh`),
* every target composes into a configuration that Kconfig accepts without
  diagnostics (an invalid value or a conflicting choice member fails),
* the ISA implied by the target name matches the composed configuration
  (`riscv64-*` must produce `CONFIG_ISA_riscv64=y`),
* composition is deterministic,
* every `CONFIG_*` symbol used by a layer exists in some `Kconfig` file,
* layers that no target uses are reported as warnings,
* override and redundant-definition counts are reported.

## Notes

* The `.config` files under `riscv64/`, `riscv32/` and `fragments/` are inputs,
  not build outputs.  The build output is `$NEMU_HOME/.config` (or
  `$KCONFIG_CONFIG`).
* `make savedefconfig` still writes `configs/defconfig`, which is a normal
  single-file defconfig usable with `make defconfig`.
* The tree was produced from the previous complete `*_defconfig` files and was
  verified target by target to produce byte-identical `.config` output.
* Some feature fragments still bundle settings that belong to a machine (for
  example `difftest/multicore` carries `MSIZE`/`FLASH_SIZE`, and
  `checkpoint/cpt` carries PMP/reset/toolchain settings).  Moving those into
  the profiles is a follow-up cleanup that does not change any composed
  configuration.
