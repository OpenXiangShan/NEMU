#include <common.h>

#define _str(x) # x
#define str(x) _str(x)

char *isa_qemu_argv[] = {
  "/usr/bin/qemu-system-mipsel",
  "-nographic", "-S", "-serial", "none", "-monitor", "none",
  "-machine", "mipssim", "-kernel", str(NEMU_HOME) "/resource/mips-elf/mips.dummy",
  NULL
};

void init_isa() {
}
