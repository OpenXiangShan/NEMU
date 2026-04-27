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

import hashlib
import json
import platform
import re
import shlex
import subprocess
import sys
from contextlib import ExitStack
from datetime import datetime
from pathlib import Path


_EVENT_ID_HASH_LEN = 8
_EVENT_ID_MAX_SLUG_LEN = 64


def _read_file_tail(path: Path, max_bytes: int = 8192) -> str:
    if not path.exists():
        return ""
    with path.open("rb") as fp:
        fp.seek(0, 2)
        size = fp.tell()
        fp.seek(max(0, size - max_bytes))
        data = fp.read()
    return data.decode("utf-8", errors="replace").strip()


def run_cmd(
    args: list[str],
    check: bool = True,
    cwd: Path | None = None,
    stdout_path: Path | None = None,
    stderr_path: Path | None = None,
) -> subprocess.CompletedProcess[str]:
    with ExitStack() as stack:
        stdout_target: int | object = subprocess.PIPE
        stderr_target: int | object = subprocess.PIPE
        if stdout_path is not None:
            ensure_dir(stdout_path.parent)
            stdout_target = stack.enter_context(stdout_path.open("w", encoding="utf-8"))
        if stderr_path is not None:
            ensure_dir(stderr_path.parent)
            stderr_target = stack.enter_context(stderr_path.open("w", encoding="utf-8"))
        try:
            proc = subprocess.run(
                args,
                stdout=stdout_target,
                stderr=stderr_target,
                text=True,
                encoding="utf-8",
                errors="replace",
                cwd=cwd,
            )
        except FileNotFoundError as exc:
            tool = args[0] if args else "<unknown>"
            raise RuntimeError(f"Command not found: {tool}") from exc
    if check and proc.returncode != 0:
        cmd = " ".join(shlex.quote(a) for a in args)
        err = proc.stderr if proc.stderr is not None else ""
        if not err and stderr_path is not None:
            err = _read_file_tail(stderr_path)
            if err:
                err = f"{err}\nstderr log: {stderr_path}"
            else:
                err = f"stderr log: {stderr_path}"
        raise RuntimeError(f"Command failed ({proc.returncode}): {cmd}\n{err}")
    return proc


def ensure_dir(path: Path) -> Path:
    path.mkdir(parents=True, exist_ok=True)
    return path


def write_text(path: Path, text: str) -> None:
    ensure_dir(path.parent)
    path.write_text(text, encoding="utf-8")


def write_json(path: Path, data: object) -> None:
    ensure_dir(path.parent)
    path.write_text(json.dumps(data, indent=2, sort_keys=True) + "\n", encoding="utf-8")


def read_json(path: Path) -> object:
    return json.loads(path.read_text(encoding="utf-8"))


def canonical_event(event: str) -> str:
    e = event.lower().strip()
    if "branch-misses" in e:
        return "branch-misses"
    if "cache-misses" in e:
        return "cache-misses"
    if "cache-references" in e:
        return "cache-references"
    if "branch-instructions" in e or "branch_instructions" in e:
        return "branches"
    if "branches" in e:
        return "branches"
    if "instructions" in e:
        return "instructions"
    if "cycles" in e and "ref-cycles" not in e:
        return "cycles"
    return event.strip()


def event_id(event: str) -> str:
    text = event.strip().lower()
    slug = re.sub(r"[^0-9A-Za-z._-]+", "_", text).strip("._-")
    if not slug or slug in {".", ".."}:
        raise ValueError(f"Unsafe event name: {event}")
    if slug == text and len(slug) <= _EVENT_ID_MAX_SLUG_LEN:
        return slug
    slug = slug[:_EVENT_ID_MAX_SLUG_LEN].rstrip("._-")
    if not slug or slug in {".", ".."}:
        raise ValueError(f"Unsafe event name: {event}")
    digest = hashlib.sha1(text.encode("utf-8")).hexdigest()[:_EVENT_ID_HASH_LEN]
    return f"{slug}-{digest}"


def fmt_num(value: float | int | None) -> str:
    if value is None:
        return "-"
    if isinstance(value, float):
        return f"{value:.4f}"
    return str(value)


def fmt_count(value: float | int | None) -> str:
    if value is None:
        return "-"
    return f"{value:.0f}"


def derive_metrics(counters: dict[str, float]) -> dict[str, float | None]:
    ins = counters.get("instructions", 0.0)
    cyc = counters.get("cycles", 0.0)
    bm = counters.get("branch-misses", 0.0)
    br = counters.get("branches", 0.0)
    cm = counters.get("cache-misses", 0.0)
    return {
        "ipc": ins / cyc if cyc > 0 else None,
        "cpi": cyc / ins if ins > 0 else None,
        "bp_mpki": 1000.0 * bm / ins if ins > 0 else None,
        "cache_mpki": 1000.0 * cm / ins if ins > 0 else None,
        "branch_miss_rate": bm / br if br > 0 else None,
    }


def parse_nemu_summary(stdout_text: str) -> tuple[int | None, int | None]:
    guest = None
    sim_freq = None
    mg = re.search(r"total guest instructions\s*=\s*([^\n]+)", stdout_text)
    if mg:
        digits = re.sub(r"\D", "", mg.group(1))
        if digits:
            guest = int(digits)
    mf = re.search(r"simulation frequency\s*=\s*([^\n]*?)\s*instr/s", stdout_text)
    if mf:
        digits = re.sub(r"\D", "", mf.group(1))
        if digits:
            sim_freq = int(digits)
    return guest, sim_freq


def progress(step: int, total: int, label: str, detail: str = "") -> None:
    suffix = f" {detail}" if detail else ""
    print(f"[{step}/{total} {label}]{suffix}", file=sys.stderr)


def make_run_id(prefix: str = "run") -> str:
    stamp = datetime.now().strftime("%Y%m%d-%H%M%S-%f")
    return f"{prefix}-{stamp}"


def read_proc_int(path: Path, default: int | None = None) -> int | None:
    try:
        return int(path.read_text(encoding="utf-8").strip())
    except Exception:
        return default


def get_cpu_model() -> str | None:
    cpuinfo = Path("/proc/cpuinfo")
    if not cpuinfo.exists():
        return None
    for line in cpuinfo.read_text(encoding="utf-8", errors="ignore").splitlines():
        if line.lower().startswith("model name"):
            return line.split(":", 1)[1].strip()
    return None


def get_host_summary() -> dict[str, str | None]:
    uname = platform.uname()
    return {
        "system": uname.system,
        "release": uname.release,
        "machine": uname.machine,
        "cpu_model": get_cpu_model(),
    }
