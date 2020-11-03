#include <common.h>
#include <sys/mman.h>
#include <pthread.h>
#include <sys/auxv.h>
#include <elf.h>

#define ALIGN_UP(a, sz) ((((uintptr_t)a) + (sz) - 1) & ~((sz) - 1))

static int (*qemu_cpu_memory_rw_debug)(void *cpu, long addr, uint8_t *buf, int len, int is_write) = NULL;
static int (*qemu_gdb_write_register)(void *cpu, uint8_t *buf, int reg) = NULL;
static int (*qemu_gdb_read_register)(void *cpu, uint8_t *buf, int reg) = NULL;
static int (*qemu_cpu_single_step)(void *cpu, int enabled) = NULL;
static void *qemu_cpu = NULL;
static int qemu_sstep_flags = 0;

void difftest_memcpy_from_dut(paddr_t dest, void *src, size_t n) {
  int ret = qemu_cpu_memory_rw_debug(qemu_cpu, dest, src, n, true);
  assert(ret == 0);
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

typedef struct {
  // the first two members from the source code of QEMU
  void *c_cpu;
  void *g_cpu;
} GDBState;

static char *strtab = NULL;
static int strtab_size = 0;
static Elf64_Sym *symtab = NULL;
static int symtab_nr_entry = 0;

typedef void (*ELF_sh_handler)(void *buf, int size, void **);
typedef struct {
  char *name;
  ELF_sh_handler h;
  void *retval;
} ELF_sh_callback;

static void build_symtab(void *buf, int size, void **retval) {
  assert(size > 0);
  symtab = buf;
  symtab_nr_entry = size / sizeof(Elf64_Sym);
}

static void build_strtab(void *buf, int size, void **retval) {
  assert(size > 0);
  strtab = buf;
  strtab_size = size;
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
        void *buf = malloc(sh[i].sh_size);
        assert(buf);
        fseek(fp, sh[i].sh_offset, SEEK_SET);
        int ret = fread(buf, sh[i].sh_size, 1, fp);
        assert(ret == 1);
        cb->h(buf, sh[i].sh_size, &cb->retval);
      }
    }
  }
  free(shstrtab);
  free(sh);
  fclose(fp);
}

static void ELF_parse(char *filename) {
  ELF_sh_callback cb_list[3] = {
    { .name = ".symtab", .h = build_symtab },
    { .name = ".strtab", .h = build_strtab },
    { .name = NULL},
  };
  ELF_sh_foreach(filename, cb_list);
  assert(symtab != NULL && strtab != NULL);
}

static void get_build_id(void *buf, int size, void **retval) {
  assert(size > 0);
  void *name, *desc;
  Elf64_Nhdr *note = buf;
  name = note->n_namesz == 0 ? NULL : buf + sizeof(*note);
  assert(strcmp(name, ELF_NOTE_GNU) == 0);
  assert(note->n_descsz == 160 / 8); // SHA1 is of 160-bit length
  desc = note->n_descsz == 0 ? NULL :
    buf + sizeof(*note) + ALIGN_UP(note->n_namesz, 4);
  char *id = malloc(note->n_descsz + 1); // +1 for '\0'
  memcpy(id, desc, note->n_descsz);
  id[note->n_descsz] = '\0';
  free(buf);
  *retval = id;
}

static char* get_debug_elf_path(char *filename) {
  const char *prefix = "/usr/bin/";
  if (strncmp(filename, prefix, strlen(prefix)) != 0) {
    return strdup(filename);
  }

  ELF_sh_callback cb_list[2] = {
    { .name = ".note.gnu.build-id", .h = get_build_id },
    { .name = NULL }
  };
  ELF_sh_foreach(filename, cb_list);

  uint8_t *id = cb_list[0].retval;
  char *path = malloc(512);
  int len = sprintf(path, "/usr/lib/debug/.build-id/%02x/", id[0]);
  int i;
  for (i = 1; i < 20; i ++) {
    len += sprintf(path + len, "%02x", id[i]);
  }
  strcat(path, ".debug");
  free(id);
  return path;
}

static uintptr_t get_sym_addr(char *sym, int type) {
  int i;
  for (i = 0; i < symtab_nr_entry; i ++) {
    if ((strcmp(strtab + symtab[i].st_name, sym) == 0) &&
        ELF64_ST_TYPE(symtab[i].st_info) == type) {
      return symtab[i].st_value;
    }
  }
  assert(0);
}

static void* get_loaded_addr(char *sym, int type) {
  static uintptr_t offset = 0;
  if (offset == 0) {
    extern int main(int, char **, char **);
    offset = (uintptr_t)main - get_sym_addr("main", STT_FUNC);
  }
  return (void *)get_sym_addr(sym, type) + offset;
}

static void hack_entry() {
  void (*qemu_main_loop_wait)(int) = get_loaded_addr("main_loop_wait", STT_FUNC);
  uintptr_t p = (uintptr_t)qemu_main_loop_wait;
  p &= ~0xfffl;
  int ret = mprotect((void *)p, 4096, PROT_READ | PROT_EXEC);
  assert(ret == 0);

  GDBState **qemu_gdbserver_state = get_loaded_addr("gdbserver_state", STT_OBJECT);
  qemu_cpu_memory_rw_debug = get_loaded_addr("cpu_memory_rw_debug", STT_FUNC);
  qemu_gdb_write_register = get_loaded_addr("gdb_write_register", STT_FUNC);
  qemu_gdb_read_register = get_loaded_addr("gdb_read_register", STT_FUNC);
  qemu_cpu_single_step = get_loaded_addr("cpu_single_step", STT_FUNC);
  void (*qemu_mutex_unlock_iothread)() = get_loaded_addr("qemu_mutex_unlock_iothread", STT_FUNC);
  int (*qemu_tcg_cpu_exec)(void *) = get_loaded_addr("cpu_exec", STT_FUNC);
  int *flags = get_loaded_addr("sstep_flags", STT_OBJECT);
  qemu_sstep_flags = *flags;

  assert(*qemu_gdbserver_state);
  qemu_cpu = (*qemu_gdbserver_state)->g_cpu;
  assert(qemu_cpu);
  printf("ok\n");

#if 1
  int len = 0x10000;
  uint8_t *buf = malloc(len);
  memset(buf, 0x40, len);
  difftest_memcpy_from_dut(0x4000, buf, len);

  uint32_t val = 0x00000000;
  qemu_gdb_write_register(qemu_cpu, (void *)&val, 10); // cs
  uint32_t pc = 0x00004000;
  int pc_idx = 8;
#else
  int len = 0x10000;
  uint32_t *buf = malloc(len * 4);
  int jjj = 0;
  for (; jjj < len; jjj ++) {
    buf[jjj] = 0x24840001; // addi a0,a0,$1
  }
  difftest_memcpy_from_dut(0x80000000, buf, len * 4);

  uint32_t pc = 0x80000000;
  int32_t pc_idx = 37;
#endif
  free(buf);

  qemu_gdb_write_register(qemu_cpu, (void *)&pc, pc_idx); // pc
  qemu_cpu_single_step(qemu_cpu, qemu_sstep_flags);
  qemu_mutex_unlock_iothread();


  clock_t t0 = clock();
  int iii = 0;
  while (1) {
  qemu_gdb_write_register(qemu_cpu, (void *)&pc, pc_idx);
  int i;
  for (i = 0; i < len; i ++) {
    qemu_tcg_cpu_exec(qemu_cpu);
//    uint32_t val = 0;
//    qemu_gdb_read_register(qemu_cpu, (void *)&val, pc_idx);
//    printf("eip = 0x%x\n", val);
//    assert(val == 0x4000 + i);
  }

  iii ++;
  if (iii == 100) break;
  }
  clock_t t1 = clock();

  uint64_t total = iii * len;
  printf("finish, freq = %lld instr/s\n", total * 1000000ull / (t1 - t0));
  while (1);
}

__attribute__((constructor)) static void myinit() {
  char *filename = (char *)getauxval(AT_EXECFN);
  assert(filename != NULL);
  char *debug_elf_path = get_debug_elf_path(filename);

  if (access(debug_elf_path, R_OK) != 0) {
    printf("File '%s' does not exist!\n", debug_elf_path);
    printf("Make sure you are using QEMU installed by apt-get, "
           "and you have already installed the debug symbol package for qemu\n");
    assert(0);
  }

  ELF_parse(debug_elf_path);
  free(debug_elf_path);

  struct {
    uint16_t opcode_movabs;
    int64_t imm;
    uint16_t instr_jmp;
  } __attribute__((packed)) code;
  assert(sizeof(code) == 12);
  code.opcode_movabs = 0xb848; // movabs $imm, %rax
  code.imm = (uintptr_t)hack_entry;
  code.instr_jmp = 0xe0ff; // jmp *%rax

  void (*qemu_main_loop_wait)(int) = get_loaded_addr("main_loop_wait", STT_FUNC);
  uintptr_t p = (uintptr_t)qemu_main_loop_wait;
  p &= ~0xfffl;
  int ret = mprotect((void *)p, 4096, PROT_READ | PROT_WRITE | PROT_EXEC);
  assert(ret == 0);
//  memcpy(code_save, qemu_main_loop_wait, sizeof(code_save));
  memcpy(qemu_main_loop_wait, &code, sizeof(code));
}
