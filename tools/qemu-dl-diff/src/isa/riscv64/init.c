#include <common.h>

char *isa_qemu_argv[] = {
  "/usr/bin/qemu-system-riscv64",
  "-nographic", "-S", "-serial", "none", "-monitor", "none",
  NULL
};

void init_isa() {
}
