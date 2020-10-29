#include <common.h>
#include <sys/mman.h>

void difftest_memcpy_from_dut(paddr_t dest, void *src, size_t n) {
  assert(0);
}

void difftest_getregs(void *r) {
  assert(0);
}

void difftest_setregs(const void *r) {
  assert(0);
}

void difftest_exec(uint64_t n) {
  assert(0);
}

void difftest_init(int port) {
  assert(0);
}

static uint8_t code_save[12];

static int mymain(int argc, char *argv[], char *envp[]) {
  printf("Here\n");
  extern int main(int, char **, char **);
  memcpy(main, code_save, sizeof(code_save));

  uintptr_t pmain = (uintptr_t)main;
  pmain &= ~0xfffl;
  int ret = mprotect((void *)pmain, 4096, PROT_READ | PROT_EXEC);
  assert(ret == 0);

  char *myargv[] = {
    "qemy-system-i386", "-nographic", "-S"
  };
  int myargc = sizeof(myargv) / sizeof(myargv[0]);
  return main(myargc, myargv, envp);
}

__attribute__((constructor)) static void myinit() {
  extern int main(int, char **, char **);
  struct {
    uint16_t opcode_movabs;
    int64_t imm;
    uint16_t instr_jmp;
  } __attribute__((packed)) code;
  assert(sizeof(code) == 12);
  assert(sizeof(code) == sizeof(code_save));
  code.opcode_movabs = 0xb848; // movabs $imm, %rax
  code.imm = (uintptr_t)mymain;
  code.instr_jmp = 0xe0ff; // jmp *%rax

  uintptr_t pmain = (uintptr_t)main;
  pmain &= ~0xfffl;
  int ret = mprotect((void *)pmain, 4096, PROT_READ | PROT_WRITE | PROT_EXEC);
  assert(ret == 0);
  memcpy(code_save, main, sizeof(code_save));
  memcpy(main, &code, sizeof(code));
  printf("main = %p, mymain = %p, offset = 0x%lx\n", main, mymain, code.imm);
}
