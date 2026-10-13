#!/usr/bin/env python3
"""Check that the XS Spike DUT follows the XS reference ISA contract."""

from pathlib import Path
import re
import sys


REPO_ROOT = Path(__file__).resolve().parents[1]
REF_CONFIG = REPO_ROOT / "configs/riscv64-xs-ref_defconfig"
DUT_CONFIG = REPO_ROOT / "configs/riscv64-xs-diff-spike_defconfig"

# The XS reference config is the source of truth. Only differences required by
# the DUT's standalone/Spike integration are allowed here. Pin both sides so a
# change to either config requires an explicit review of the exception.
MISSING = "<unset>"
EXPECTED_DUT_OVERRIDES = {
    "CLINT_LOCAL_TIMER_INTERRUPT": ("n", "y", "standalone DUT timer model"),
    "CYCLES_PER_MTIME_TICK": (MISSING, "128", "standalone DUT timer setting"),
    "CUSTOM_XVEXP2": ("y", "n", "not implemented by the current Spike target"),
    "CUSTOM_XVEXP2_BF16": (
        "y",
        MISSING,
        "depends on the unsupported XVEXP2 extension",
    ),
    "DIFFTEST_DIRTY_FS_VS": (MISSING, "n", "DiffTest-only control"),
    "RV_PMA_CHECK": ("y", MISSING, "disabled by PERF_OPT in the DUT config"),
    "RV_PMP_CHECK": ("y", MISSING, "disabled by PERF_OPT in the DUT config"),
    "RV_SHLCOFIDELEG": ("y", "n", "not implemented by the current Spike target"),
    "WFI_TIMEOUT_TICKS": (MISSING, "8192", "standalone DUT timer setting"),
}


def parse_isa_section(path: Path) -> dict[str, str]:
    try:
        lines = path.read_text(encoding="utf-8").splitlines()
    except OSError as exc:
        raise RuntimeError(f"{path}: cannot read config: {exc}") from exc
    try:
        begin = lines.index("# ISA-dependent Options for riscv64")
        end = lines.index("# end of ISA-dependent Options for riscv64")
    except ValueError as exc:
        raise RuntimeError(f"{path}: missing ISA configuration section") from exc

    values: dict[str, str] = {}
    assignment = re.compile(r"CONFIG_([A-Za-z0-9_]+)=(.*)")
    unset = re.compile(r"# CONFIG_([A-Za-z0-9_]+) is not set")
    for line in lines[begin + 1 : end]:
        match = assignment.fullmatch(line)
        if match:
            values[match.group(1)] = match.group(2)
            continue
        match = unset.fullmatch(line)
        if match:
            values[match.group(1)] = "n"
    return values


def main() -> int:
    try:
        ref = parse_isa_section(REF_CONFIG)
        dut = parse_isa_section(DUT_CONFIG)
    except RuntimeError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 2

    mismatches: list[str] = []
    # Include allowlisted keys even if a future config removes the option from
    # both files, so an exception cannot silently disappear.
    for key in sorted(set(ref) | set(dut) | set(EXPECTED_DUT_OVERRIDES)):
        ref_value = ref.get(key, MISSING)
        dut_value = dut.get(key, MISSING)
        if ref_value == dut_value:
            if key in EXPECTED_DUT_OVERRIDES:
                expected = EXPECTED_DUT_OVERRIDES[key]
                mismatches.append(
                    f"CONFIG_{key}: expected DUT override "
                    f"ref={expected[0]} dut={expected[1]}, "
                    f"found ref={ref_value} dut={dut_value}"
                )
            continue

        expected = EXPECTED_DUT_OVERRIDES.get(key)
        if expected and (ref_value, dut_value) == expected[:2]:
            print(
                f"expected DUT override: CONFIG_{key}: "
                f"ref={ref_value} dut={dut_value} ({expected[2]})"
            )
            continue
        mismatches.append(f"CONFIG_{key}: ref={ref_value} dut={dut_value}")

    if mismatches:
        print(
            "Unexpected ISA differences from reference config "
            f"{REF_CONFIG.relative_to(REPO_ROOT)}:",
            file=sys.stderr,
        )
        for mismatch in mismatches:
            print(f"  {mismatch}", file=sys.stderr)
        return 1

    print(
        "DUT ISA configuration follows reference config "
        f"{REF_CONFIG.relative_to(REPO_ROOT)}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
