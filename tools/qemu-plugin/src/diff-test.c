#include <common.h>
#include <sys/mman.h>
#include <pthread.h>
#include <sys/auxv.h>
#include <elf.h>

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
static int (*qemu_main)(int, char **, char **) = NULL;

static void* qemu_thread(void *arg) {
  char *myargv[] = {
    "qemy-system-i386", "-nographic", "-S"
  };
  int myargc = sizeof(myargv) / sizeof(myargv[0]);
  qemu_main(myargc, myargv, arg);
  assert(0);
}

static char *strtab = NULL;
static int strtab_size = 0;
static Elf64_Sym *symtab = NULL;
static int symtab_nr_entry = 0;

typedef void (*ELF_sh_handler)(FILE *fp, Elf64_Shdr *);
typedef struct {
  char *name;
  ELF_sh_handler h;
} ELF_sh_callback;

static void build_symtab(FILE *fp, Elf64_Shdr *sh) {
  assert(sh->sh_size > 0);
  symtab = malloc(sh->sh_size);
  assert(symtab);
  fseek(fp, sh->sh_offset, SEEK_SET);
  int ret = fread(symtab, sh->sh_size, 1, fp);
  assert(ret == 1);
  symtab_nr_entry = sh->sh_size / sizeof(Elf64_Sym);
}

static void build_strtab(FILE *fp, Elf64_Shdr *sh) {
  assert(sh->sh_size > 0);
  strtab = malloc(sh->sh_size);
  assert(strtab);
  fseek(fp, sh->sh_offset, SEEK_SET);
  int ret = fread(strtab, sh->sh_size, 1, fp);
  assert(ret == 1);
  strtab_size = sh->sh_size;
}

static void ELF_sh_foreach(char *filename, ELF_sh_callback *cb_list) {
  FILE *fp = fopen(filename, "r");
  assert(fp != NULL);

  Elf64_Ehdr elf;
  fread(&elf, sizeof(elf), 1, fp);
  assert(elf.e_ident[EI_MAG0] == ELFMAG0);
  assert(elf.e_ident[EI_MAG1] == ELFMAG1);
  assert(elf.e_ident[EI_MAG2] == ELFMAG2);
  assert(elf.e_ident[EI_MAG3] == ELFMAG3);

  Elf64_Shdr *sh = NULL;
  int shdr_size = sizeof(*sh) * elf.e_shnum;
  sh = malloc(shdr_size);
  assert(sh);
  fseek(fp, elf.e_shoff, SEEK_SET);
  int ret = fread(sh, shdr_size, 1, fp);
  assert(ret == 1);

  int shstrtab_size = sh[elf.e_shstrndx].sh_size;
  assert(shstrtab_size > 0);
  char *shstrtab = malloc(shstrtab_size);
  assert(shstrtab);
  fseek(fp, sh[elf.e_shstrndx].sh_offset, SEEK_SET);
  ret = fread(shstrtab, sh[elf.e_shstrndx].sh_size, 1, fp);
  assert(ret == 1);

  int i;
  for (i = 0; i < elf.e_shnum; i ++) {
    ELF_sh_callback *cb;
    for (cb = cb_list; cb->name != NULL; cb ++) {
      if (strcmp(shstrtab + sh[i].sh_name, cb->name) == 0) {
        cb->h(fp, &sh[i]);
      }
    }
  }
  free(shstrtab);
  free(sh);
  fclose(fp);
}

static void parseELF(char *filename) {
  ELF_sh_callback cb_list[3] = {
    { .name = ".dynsym", .h = build_symtab },
    { .name = ".dynstr", .h = build_strtab },
    { .name = NULL},
  };
  ELF_sh_foreach(filename, cb_list);
  assert(symtab != NULL && strtab != NULL);
}

static uintptr_t get_sym_addr(char *sym) {
  int i;
  for (i = 0; i < symtab_nr_entry; i ++) {
    if ((strcmp(strtab + symtab[i].st_name, sym) == 0) &&
        ELF64_ST_TYPE(symtab[i].st_info) == STT_FUNC) {
      return symtab[i].st_value;
    }
  }
  assert(0);
}

static uintptr_t get_loaded_addr(char *sym) {
  static uintptr_t main_addr = 0;
  if (main_addr == 0) {
    main_addr = get_sym_addr("main");
  }
  return get_sym_addr(sym) + ((uintptr_t)qemu_main - main_addr);
}

static int mymain(int argc, char *argv[], char *envp[]) {
  memcpy(qemu_main, code_save, sizeof(code_save));

  uintptr_t pmain = (uintptr_t)qemu_main;
  pmain &= ~0xfffl;
  int ret = mprotect((void *)pmain, 4096, PROT_READ | PROT_EXEC);
  assert(ret == 0);

  pthread_t thread;
  pthread_create(&thread, NULL, qemu_thread, envp);

  char *filename = (char *)getauxval(AT_EXECFN);
  assert(filename != NULL);

  parseELF(filename);

  bool (*runstate_check)(int) = (void *)get_loaded_addr("runstate_check");
  int RUN_STATE_PRELAUNCH = 6;
  while (!runstate_check(RUN_STATE_PRELAUNCH)) {
    usleep(1);
  }
  printf("ok\n");

  while (1);
}

__attribute__((constructor)) static void myinit() {
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

  extern int main(int, char **, char **);
  qemu_main = main;
  uintptr_t pmain = (uintptr_t)qemu_main;
  pmain &= ~0xfffl;
  int ret = mprotect((void *)pmain, 4096, PROT_READ | PROT_WRITE | PROT_EXEC);
  assert(ret == 0);
  memcpy(code_save, qemu_main, sizeof(code_save));
  memcpy(qemu_main, &code, sizeof(code));
  printf("main = %p, mymain = %p, offset = 0x%lx\n", qemu_main, mymain, code.imm);
}
