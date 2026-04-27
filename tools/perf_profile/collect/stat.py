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

from pathlib import Path

from ..catalog import GLOBAL_EVENTS
from ..parse import parse_perf_stat
from ..util import run_cmd


def collect_perf_stat(binary: Path, workload: Path, nemu_args: list[str], out_dir: Path) -> dict[str, object]:
    out_dir.mkdir(parents=True, exist_ok=True)
    run_dir = out_dir.parent.parent
    stdout_path = out_dir / "perf.stat.stdout.log"
    stderr_path = out_dir / "perf.stat.stderr.log"
    cmd = ["perf", "stat", "--all-user"]
    for event in GLOBAL_EVENTS:
        cmd += ["-e", event]
    cmd += ["--", str(binary), *nemu_args, str(workload)]
    proc = run_cmd(cmd, check=False, stdout_path=stdout_path, stderr_path=stderr_path)
    parsed = parse_perf_stat(stderr_path.read_text(encoding="utf-8", errors="replace"))
    parsed["returncode"] = proc.returncode
    parsed["command"] = cmd
    parsed["stdout_log"] = str(stdout_path.relative_to(run_dir))
    parsed["stderr_log"] = str(stderr_path.relative_to(run_dir))
    return parsed
