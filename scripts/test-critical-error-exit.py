#!/usr/bin/env python3
"""Check standalone exits with riscv64-xs_defconfig (Smrnmi, NMIE initially 0).

Usage: python3 scripts/test-critical-error-exit.py build/riscv64-nemu-interpreter
Add --ref PATH to also check a riscv64-xs-ref shared library built with
CONFIG_REF_STATUS=y. No RISC-V compiler or external guest image is needed.
"""

import argparse
import ctypes
from pathlib import Path
import struct
import subprocess
import tempfile


def check_exit(binary, image, expected_status, expected_log, commands=None):
    args = [str(binary)]
    if commands is None:
        args.append("-b")
    result = subprocess.run(
        args + [str(image)], input=commands, capture_output=True, text=True, timeout=10
    )
    output = result.stdout + result.stderr
    if result.returncode != expected_status or expected_log not in output:
        raise AssertionError(
            f"{image.stem}: expected status {expected_status} and {expected_log!r}, "
            f"got status {result.returncode}:\n{output}"
        )
    if expected_status and "HIT GOOD TRAP" in output:
        raise AssertionError(f"{image.stem}: failure reported as GOOD TRAP:\n{output}")
    print(f"PASS: {image.stem} (exit {result.returncode})")


def check_shared(path):
    ref = ctypes.CDLL(str(path.resolve()))
    ref.difftest_set_ramsize.argtypes = [ctypes.c_size_t]
    ref.difftest_set_ramsize.restype = None
    ref.difftest_init.argtypes = []
    ref.difftest_init.restype = None
    ref.difftest_raise_intr.argtypes = [ctypes.c_uint64]
    ref.difftest_raise_intr.restype = None
    ref.difftest_raise_critical_error.argtypes = []
    ref.difftest_raise_critical_error.restype = ctypes.c_bool
    ref.difftest_status.argtypes = []
    ref.difftest_status.restype = ctypes.c_int

    ref.difftest_set_ramsize(16 * 1024 * 1024)
    ref.difftest_init()
    assert not ref.difftest_raise_critical_error(), "critical error set before trap"
    assert ref.difftest_status() == 0, "reference not running before trap"
    ref.difftest_raise_intr(2)  # Illegal instruction while mnstatus.NMIE is clear.
    assert ref.difftest_raise_critical_error(), "critical error not exposed to difftest"
    assert ref.difftest_status() == 0, "critical error terminated the shared reference"
    print("PASS: shared reference reports critical error without terminating")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("binary", type=Path)
    parser.add_argument("--ref", type=Path)
    args = parser.parse_args()
    binary = args.binary.resolve()
    with tempfile.TemporaryDirectory(prefix="nemu-critical-exit-") as directory:
        directory = Path(directory)
        critical = directory / "critical.bin"
        critical.write_bytes(bytes(4))
        check_exit(binary, critical, 1, "HIT CRITICAL ERROR")

        # addi a0, zero, result; nemu_trap. These instructions cannot trap.
        for name, code, marker in (("good", 0, "HIT GOOD TRAP"), ("bad", 1, "HIT BAD TRAP")):
            image = directory / f"{name}.bin"
            image.write_bytes(struct.pack("<II", (code << 20) | 0x513, 0x6B))
            check_exit(binary, image, code, marker)
        quit_image = directory / "quit.bin"
        quit_image.write_bytes(bytes(4))
        check_exit(binary, quit_image, 0, "NEMU exit with good state", commands="si 1\nq\n")

    if args.ref:
        check_shared(args.ref)


if __name__ == "__main__":
    main()
