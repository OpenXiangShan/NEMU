#!/usr/bin/env python3

import sys
from pathlib import Path

if len(sys.argv) != 3:
  sys.exit(f"Usage: {sys.argv[0]} INPUT_GCPT_BIN OUTPUT_C")

input_path = Path(sys.argv[1])
output_path = Path(sys.argv[2])
data = input_path.read_bytes()

restorer_size = int.from_bytes(data[4:8], "little")
restorer = data[:restorer_size]
lines = [
    "#include <stddef.h>",
    "#include <stdint.h>",
    "",
    "const uint8_t gcpt_restorer_bin[] = {",
]

for offset in range(0, len(restorer), 12):
  lines.append("  " + ", ".join(f"0x{byte:02x}" for byte in restorer[offset:offset + 12]) + ",")

lines += [
    "};",
    "const size_t gcpt_restorer_size = sizeof(gcpt_restorer_bin);",
]
output_path.parent.mkdir(parents=True, exist_ok=True)
output_path.write_text("\n".join(lines) + "\n")
