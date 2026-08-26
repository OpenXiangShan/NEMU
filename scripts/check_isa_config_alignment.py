#!/usr/bin/env python3
"""Check the ISA contract shared by the XS ref and Spike DiffTest configs."""

from pathlib import Path
import re
import sys


REF_CONFIG = Path("configs/riscv64-xs-ref_defconfig")
DUT_CONFIG = Path("configs/riscv64-xs-diff-spike_defconfig")

# These differences are intentional and must remain visible in this allowlist.
# Each entry fixes both expected values so a change on either side still fails.
MISSING = "<unset>"
EXPECTED_DIFFERENCES = {
    "CLINT_LOCAL_TIMER_INTERRUPT": ("n", "y", "different timer integration"),
    "CUSTOM_XVEXP2": ("y", "n", "not implemented by the current Spike target"),
    "CUSTOM_XVEXP2_BF16": ("y", MISSING, "depends on XVEXP2"),
    "CYCLES_PER_MTIME_TICK": (MISSING, "128", "DUT timer integration setting"),
    "DIFFTEST_DIRTY_FS_VS": (MISSING, "n", "DiffTest-only control"),
    "GEILEN": ("5", MISSING, "AIA/IMSIC is not enabled in the DUT config"),
    "RV_AIA": ("y", MISSING, "AIA requires Smcsrind, which remains disabled"),
    "RV_IMSIC": ("y", MISSING, "AIA requires Smcsrind, which remains disabled"),
    "RV_PMA_CHECK": ("y", MISSING, "disabled by PERF_OPT in the DUT config"),
    "RV_PMP_CHECK": ("y", MISSING, "disabled by PERF_OPT in the DUT config"),
    "RV_SHLCOFIDELEG": ("y", "n", "not implemented by the current Spike target"),
    "RV_SMCDELEG": ("y", MISSING, "depends on Smcsrind and is disabled with it"),
    "RV_SMCSRIND": ("y", "n", "kept disabled by the DUT configuration contract"),
    "WFI_TIMEOUT_TICKS": (MISSING, "8192", "DUT timer integration setting"),
}


def parse_isa_section(path: Path) -> dict[str, str]:
    lines = path.read_text(encoding="utf-8").splitlines()
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
    ref = parse_isa_section(REF_CONFIG)
    dut = parse_isa_section(DUT_CONFIG)
    mismatches = []
    for key in sorted(set(ref) | set(dut)):
        ref_value = ref.get(key, MISSING)
        dut_value = dut.get(key, MISSING)
        if ref_value == dut_value:
            continue
        expected = EXPECTED_DIFFERENCES.get(key)
        if expected and (ref_value, dut_value) == expected[:2]:
            print(
                f"expected ISA difference: CONFIG_{key}: "
                f"ref={ref_value} dut={dut_value} ({expected[2]})"
            )
        else:
            mismatches.append(
                f"CONFIG_{key}: ref={ref_value} dut={dut_value}"
            )

    if mismatches:
        print("Unexpected ISA configuration differences:", file=sys.stderr)
        for mismatch in mismatches:
            print(f"  {mismatch}", file=sys.stderr)
        return 1

    print("ISA ref/DUT configuration contract check passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
