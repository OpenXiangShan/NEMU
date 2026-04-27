#***************************************************************************************
# Copyright (c) 2026 Institute of Computing Technology, Chinese Academy of Sciences
#
# NEMU is licensed under Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#          http://license.coscl.org.cn/MulanPSL2
#
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
# EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
# MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
#
# See the Mulan PSL v2 for more details.
#***************************************************************************************/

from __future__ import annotations

import re
import subprocess
from pathlib import Path

from .models import WarningEntry, to_data
from .util import get_host_summary, read_proc_int, run_cmd


def _version_line(cmd: list[str]) -> str | None:
    try:
        proc = run_cmd(cmd, check=False)
    except RuntimeError:
        return None
    text = proc.stdout.strip() or proc.stderr.strip()
    if not text:
        return None
    return text.splitlines()[0]


def _probe_symbolization(binary: Path) -> bool | None:
    try:
        proc = subprocess.Popen(
            ["nm", "-n", "--defined-only", str(binary)],
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
            text=True,
            encoding="utf-8",
            errors="replace",
        )
    except OSError:
        return None

    probe_addr: str | None = None
    assert proc.stdout is not None
    try:
        for line in proc.stdout:
            match = re.match(r"^(?P<addr>[0-9a-fA-F]+)\s+\S\s+\S+$", line.strip())
            if match is None:
                continue
            probe_addr = match.group("addr")
            if int(probe_addr, 16) != 0:
                proc.terminate()
                break
    finally:
        proc.stdout.close()
        try:
            proc.wait(timeout=1)
        except subprocess.TimeoutExpired:
            proc.kill()
            proc.wait()
    if probe_addr is None:
        return False

    try:
        probe = run_cmd(["addr2line", "-f", "-C", "-e", str(binary), f"0x{probe_addr}"], check=False)
    except RuntimeError:
        return False
    if probe.returncode != 0:
        return False

    lines = [line.strip() for line in probe.stdout.splitlines() if line.strip()]
    if len(lines) < 2:
        return False
    function, location = lines[0], lines[1]
    return function != "??" and location not in {"??:0", "??:?"}


def run_doctor(binary: Path, workload: Path | None) -> dict[str, object]:
    warnings: list[WarningEntry] = []
    checks = {
        "python_version": _version_line(["python3", "--version"]),
        "perf_version": _version_line(["perf", "--version"]),
        "addr2line_version": _version_line(["addr2line", "--version"]),
        "nm_version": _version_line(["nm", "--version"]),
        "perf_event_paranoid": read_proc_int(Path("/proc/sys/kernel/perf_event_paranoid")),
        "perf_event_max_sample_rate": read_proc_int(Path("/proc/sys/kernel/perf_event_max_sample_rate")),
        "binary_exists": binary.exists(),
        "workload_exists": workload.exists() if workload is not None else None,
    }

    if not binary.exists():
        warnings.append(WarningEntry("binary_missing", f"binary not found: {binary}", "error"))
    if workload is not None and not workload.exists():
        warnings.append(WarningEntry("workload_missing", f"workload not found: {workload}", "error"))
    if checks["perf_version"] is None:
        warnings.append(WarningEntry("perf_missing", "perf not found in PATH.", "error"))
    if checks["addr2line_version"] is None:
        warnings.append(WarningEntry("addr2line_missing", "addr2line not found in PATH.", "error"))
    if checks["nm_version"] is None:
        warnings.append(WarningEntry("nm_missing", "nm not found in PATH.", "error"))

    if checks["perf_event_paranoid"] is not None and checks["perf_event_paranoid"] > 1:
        warnings.append(
            WarningEntry(
                "perf_permissions",
                "perf_event_paranoid is high; user-space sampling may be restricted or incomplete.",
            )
        )

    symbolization = None
    if binary.exists() and checks["addr2line_version"] is not None and checks["nm_version"] is not None:
        symbolization = _probe_symbolization(binary)
        if symbolization is False:
            warnings.append(
                WarningEntry(
                    "symbolization_probe_failed",
                    "addr2line did not resolve source locations from the binary; reports may show ??:0.",
                )
            )

    report = {
        "host": get_host_summary(),
        "checks": {**checks, "symbolization_probe_ok": symbolization},
        "warnings": [to_data(w) for w in warnings],
    }
    return report
