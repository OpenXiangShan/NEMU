---
name: nemu-delivery-standard
description: Enforce local AI-agent delivery standards for NEMU changes, requiring a NEMU DiffTest-with-Spike Linux smoke run before handoff whenever the change can affect build, execution, CI, DiffTest, checkpoint, memory, device, or RISC-V behavior.
---

# NEMU Delivery Standard

Use this skill before handing off NEMU code or harness changes.

Use `agent/architecture.md` as the source of truth for module boundaries and execution-sensitive data flows.

## Commit Scope

Each AI-authored commit must modify only one NEMU module. If a task needs multiple modules, split it into stacked commits or stacked PRs.

Use the module boundaries listed in `agent/skills/nemu-harness.md`.

Allowed cross-module exceptions in one commit:

- a public interface change plus its direct implementation in the same module family, such as `include/memory/` with `src/memory/`
- a test or CI harness update that verifies the same module change
- mechanical rename/move with no behavior change

When using an exception, explain it in the commit body with `Constraint:`.

## Commit Format

Every AI-authored commit must use NEMU's existing Conventional Commit title style plus the repository Lore trailers:

```text
<type>(<scope>): <imperative intent>

<commit introduction: module boundary, motivation, and expected unchanged behavior>

Constraint: <external constraint or module boundary that shaped the change>
Rejected: <alternative considered> | <reason>
Confidence: <low|medium|high>
Scope-risk: <narrow|moderate|broad>
Directive: <future-maintainer guidance>
Tested: <commands actually run>
Not-tested: <known verification gaps>
```

Allowed `type` values for AI-authored commits:

- `fix`: correct wrong behavior
- `feat`: add supported behavior or coverage
- `perf`: improve performance without changing behavior
- `refactor`: restructure without intended behavior change
- `test`: add or adjust tests only
- `ci`: change CI or automation
- `docs`: change documentation or agent guidance
- `chore`: mechanical maintenance that does not fit the above

The `scope` must be a NEMU module name or a well-established local scope, for example `ci`, `agent`, `checkpoint`, `difftest`, `monitor`, `riscv64`, `memory`, `device`, `tcache`, or `Makefile`.

The title must be imperative, lower-case after the colon, and specific. Good examples:

- `fix(difftest): preserve store queue bounds during replay`
- `ci(agent): document NEMU delivery gates`
- `docs(agent): define AI commit boundaries`

Bad examples:

- `fix`
- `update files`
- `misc changes`
- `Document agent maintenance standards for NEMU`

The commit introduction/body must state:

- the touched module
- why the change is needed
- why it belongs in one commit
- what behavior is expected to stay unchanged

## Required Gate

For any change that can affect NEMU build, execution, CI, DiffTest, checkpoint restore, memory, devices, or RISC-V behavior, the delivery claim must include a local Linux smoke run under NEMU DiffTest with Spike.

Default command shape:

```bash
NEMU_HOME=$PWD make riscv64-xs-diff-spike_defconfig
NEMU_HOME=$PWD make -j"$(nproc)"
make init-ready-to-run
./build/riscv64-nemu-interpreter \
  -b --diff ./ready-to-run/spike-xiangshan-ref.so \
  ./workloads/linux/hello/fw_payload.bin
```

If the workload directory is not initialized, first use the repository's normal workload setup path from CI or `make init-ready-to-run` as applicable. Do not silently substitute a non-Linux bare-metal test for this required gate.

## Acceptable Result

The Linux smoke passes when NEMU exits without a DiffTest mismatch, crash, assertion, or bad trap. Capture the exact command and the relevant final output in the handoff.

## When It Cannot Run

If the local environment lacks workloads, ready-to-run artifacts, Spike shared object, compiler, or time budget, report this as `Not-tested` with the exact blocker and run the strongest available fallback:

- `git diff --check`
- `bash -n` for changed shell scripts
- `python3 -m py_compile` for changed Python scripts
- `NEMU_HOME=$PWD make <changed defconfig>` for config-sensitive changes
- focused unit/smoke command relevant to the changed subsystem

Do not claim the change is fully delivered if the NEMU DiffTest-with-Spike Linux smoke was skipped.

## Handoff Wording

Always state:

- the branch and commit
- the touched module and commit introduction
- the Linux DiffTest smoke command and result, or the blocker
- any weaker fallback checks that ran
- remaining risk if the required gate did not run
