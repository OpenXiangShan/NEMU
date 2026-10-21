# NEMU configuration files

NEMU is configured with Kconfig (`Kconfig`, `.config`, `make menuconfig`).
This directory holds the *named* configurations: one entry point per supported
board or variant, plus the layers those entry points are built from.

The configurations used to be stored as 50+ complete `.config` dumps.  A
setting shared by twenty boards was therefore written down twenty times and a
change to the common platform had to be repeated in every file.  The files are
now split into a base per board, board variants and feature fragments that are
composed on demand, in the same spirit as the Linux kernel's board
`defconfig`s plus config fragments (`arch/*/configs/*.config` and
`kernel/configs/*.config`).

## Layout

```
configs/
├── riscv64/
│   ├── isa.config                  ISA baseline (included by every riscv64 board)
│   ├── xs/                         XiangShan family
│   │   ├── base.config             simulation board      -> riscv64-xs
│   │   ├── ref.config              difftest reference    -> riscv64-xs-ref
│   │   ├── fpga.config             FPGA board            -> riscv64-xs-fpga
│   │   ├── ahead.config            "ahead" board         -> riscv64-xs-ahead-ref
│   │   └── novga.config            lean board, no VGA    -> riscv64-xs-novga
│   ├── southlake/
│   │   ├── base.config             SouthLake board       -> riscv64-xs-southlake
│   │   ├── ref.config              difftest reference    -> riscv64-xs-southlake-ref
│   │   └── fpga.config             FPGA board            -> riscv64-xs-southlake-fpga
│   ├── nutshell/
│   │   └── base.config             NutShell board        -> riscv64-nutshell
│   ├── nanhu/
│   │   └── base.config             NanHu board           -> riscv64-nanhu
│   ├── yanqihu/
│   │   └── base.config             YanQiHu board         -> riscv64-yanqihu
│   └── gem5/
│       ├── base.config             GEM5 reference        -> riscv64-gem5-ref
│       └── ci.config               GEM5 CI reference     -> riscv64-gem5-ci-*-ref
├── riscv32/
│   └── isa.config                  ISA baseline for riscv32-pa
├── fragments/                      features shared by several boards
│   ├── difftest/
│   │   ├── ref-share.config        build as a SHARE-mode difftest reference
│   │   ├── spike.config            diff against Spike
│   │   ├── multicore.config        multicore difftest
│   │   └── ctrl.config             SHARE-mode controller (CUTE)
│   ├── checkpoint/
│   │   ├── cpt.config              checkpoint taking (ZSTD images)
│   │   ├── libcheckpoint.config    restore through LibCheckpoint
│   │   └── flash.config            flash image for checkpoint boots
│   ├── debug/debug.config          instruction tracing and watchpoints
│   ├── toolchain/clang.config      build NEMU with clang
│   ├── mem/dedup.config            external COW memory for GEM5
│   └── ext/                        AME, MPT, bitmap, Zicfilp, tensor, KMH-v3 CSRs
└── <target>_defconfig              entry points, e.g. riscv64-xs-ref_defconfig
```

Three kinds of layer, all plain `.config` syntax:

* **Board base** — `configs/<isa>/<board>/base.config`.  The processor model,
  memory map, on-chip devices and ISA extensions of one board.  It starts with
  `# fragment: ../isa.config`, so a board base alone is a complete
  configuration.
* **Board variant** — `<board>/<variant>.config`.  Turns that board into a
  variant (difftest reference, FPGA board, ...).  It only carries the settings
  that differ from the board base; the targets list it *after* the base.
* **Feature fragment** — `configs/fragments/<area>/<name>.config`.  A feature
  that several unrelated boards can enable (difftest mode, checkpointing,
  debug, compiler, memory model, ISA extension).  It only carries the settings
  that *all* targets using it agree on.

The naming follows Linux: `*_defconfig` is an entry point, `*.config` is a
layer that is merged into a configuration.  Unlike Linux we keep the entry
points at the top of `configs/`, because `make <name>_defconfig` and existing
scripts address them as `configs/<name>_defconfig`.

## Composition

`make <target>_defconfig` merges the layers of the target and installs the
result with Kconfig, exactly like a full `.config` was used before:

```sh
make riscv64-xs-ref_defconfig
make -j"$(nproc)"
```

The merge itself is a standalone script:

```sh
# which layers does a target use?
scripts/merge_config.sh --list configs/riscv64-xs-ref_defconfig

# merge them into a file (the same file make feeds to Kconfig)
scripts/merge_config.sh -o /tmp/nemu.config configs/riscv64-xs-ref_defconfig

# ad-hoc layers without a target file
scripts/merge_config.sh -o /tmp/nemu.config \
    configs/riscv64/xs/base.config configs/fragments/difftest/spike.config
```

Rules:

* Layers are merged in the order in which the target lists them.  For each
  `CONFIG_*` symbol the last definition wins, so a later layer (or the target
  itself) overrides an earlier one.
* A layer may pull in another one with a `# fragment: <path>` line (path
  relative to the file).  Board bases use this for their ISA baseline; targets
  do not need it.
* Symbols that are not mentioned keep their Kconfig default.
* The merged result is ordinary `.config` syntax.

## Target files

A target is a small file that lists its layers in order and carries the
settings that are unique to it:

```
# configs/riscv64-xs-dual-ref_defconfig
#
# fragment: riscv64/xs/base.config
# fragment: riscv64/xs/ref.config
# fragment: fragments/difftest/multicore.config
#
# Target-specific overrides.
CONFIG_MULTICORE_DIFF=y
```

The target name is the `*_defconfig` suffix, i.e. the file above is built with
`make riscv64-xs-dual-ref_defconfig` and produces `$NEMU_HOME/.config`.

## Where does a setting belong?

* Shared by every board of an ISA (ISA choice, reset offset):
  `configs/<isa>/isa.config`.
* Part of one board (memory map, devices, extension set, core model):
  the board's `base.config`.
* A variant of that board that only differs in a few settings:
  a file next to the board's `base.config`, listed by the targets after it.
* A feature several unrelated boards can enable: a fragment under
  `configs/fragments/`.
* Only one target needs it: the `# Target-specific overrides.` section of that
  target.

When two or more targets start to duplicate the same setting, promote it to
the board base or fragment they have in common; the merge keeps the result
identical.

## Adding a target

1. Pick the board base (and, if needed, a board variant), then the feature
   fragments you want.
2. Create `configs/<name>_defconfig` listing the layers in order, plus any
   target-specific settings.
3. `make <name>_defconfig && make -j` and check the result.
4. `make config-check` (or `scripts/check_configs.sh`) to verify the whole tree.

## Checking the tree

`make config-check` (= `scripts/check_configs.sh`) is part of CI and verifies
that

* every target composes into a configuration that Kconfig accepts,
* composition is deterministic,
* every `CONFIG_*` symbol used by a layer exists in some `Kconfig` file
  (a typo would otherwise be ignored silently),
* every layer is used by at least one target.

## Notes

* The `.config` files under `riscv64/`, `riscv32/` and `fragments/` are inputs,
  not build outputs.  The build output is `$NEMU_HOME/.config` (or
  `$KCONFIG_CONFIG`).
* `make savedefconfig` still writes `configs/defconfig`, which is a normal
  single-file defconfig usable with `make defconfig`.
* The tree was produced from the previous complete `*_defconfig` files and was
  verified target by target to produce byte-identical `.config` output.
