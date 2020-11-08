#define _GNU_SOURCE
#include <common.h>
#include <sys/mman.h>
#include <elf.h>
#include <link.h>

#define ALIGN_UP(a, sz) ((((uintptr_t)a) + (sz) - 1) & ~((sz) - 1))

static char *strtab = NULL;
static int strtab_size = 0;
static Elf64_Sym *symtab = NULL;
static int symtab_nr_entry = 0;
static uintptr_t elf_base = 0;

static void mprotect_page(uintptr_t addr, int prot) {
  addr &= ~0xfffl;
  int ret = mprotect((void *)addr, 4096, prot);
  assert(ret == 0);
}

typedef struct {
  char *name;
  uintptr_t base;
  uintptr_t tls_offset_diff;
  int next_tls_modid;
  char *debug_elf_path;
} Info;

typedef int (*ELF_sh_handler)(void *buf, int size, void **);
typedef struct {
  char *name;
  ELF_sh_handler h;
  void *userdata;
} ELF_sh_callback;

static int build_symtab(void *buf, int size, void **userdata) {
  assert(size > 0);
  symtab = buf;
  symtab_nr_entry = size / sizeof(Elf64_Sym);
  return 0;
}

static int build_strtab(void *buf, int size, void **userdata) {
  assert(size > 0);
  strtab = buf;
  strtab_size = size;
  return 0;
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
        int needfree = cb->h(buf, sh[i].sh_size, &cb->userdata);
        if (needfree) free(buf);
      }
    }
  }
  free(shstrtab);
  free(sh);
  fclose(fp);
}

static void parse_debug_elf(char *filename) {
  ELF_sh_callback cb_list[3] = {
    { .name = ".symtab", .h = build_symtab },
    { .name = ".strtab", .h = build_strtab },
    { .name = NULL},
  };
  ELF_sh_foreach(filename, cb_list);
  assert(symtab != NULL && strtab != NULL);
}

static int fix_tls_offset(void *buf, int size, void **userdata) {
  Info *info = *userdata;
  Elf64_Rela *rela_dyn = NULL;
  int nr_rela_dyn = 0;

  Elf64_Dyn *dyn = buf;
  int nr_dyn = size / sizeof(dyn[0]);
  int i;
  for (i = 0; i < nr_dyn; i ++ ) {
    switch (dyn[i].d_tag) {
      case DT_RELA:    rela_dyn = (void *)info->base + dyn[i].d_un.d_ptr; break;
      case DT_RELASZ:  nr_rela_dyn = dyn[i].d_un.d_val / sizeof(rela_dyn[0]); break;
      case DT_RELAENT: assert(dyn[i].d_un.d_val == sizeof(rela_dyn[0])); break;
    }
  }

  for (i = 0; i < nr_rela_dyn; i ++) {
    if (ELF64_R_TYPE(rela_dyn[i].r_info) == R_X86_64_TPOFF64) {
      uintptr_t ptr = info->base + rela_dyn[i].r_offset;
      mprotect_page(ptr, PROT_READ|PROT_WRITE);
      *(uintptr_t *)ptr += info->tls_offset_diff;
      mprotect_page(ptr, PROT_READ);
    }
  }
  return 1;
}

static int get_build_id(void *buf, int size, void **userdata) {
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
  *userdata = id;
  return 1;
}

static void parse_origin_elf(Info *info) {
  ELF_sh_callback cb_list[3] = {
    { .name = ".dynamic", fix_tls_offset },
    { .name = ".note.gnu.build-id", .h = get_build_id },
    { .name = NULL }
  };

  const char *prefix = "/usr/bin/";
  bool need_dbgsym = (strncmp(info->name, prefix, strlen(prefix)) == 0);
  if (!need_dbgsym) {
    cb_list[1].name = NULL;
  }

  cb_list[0].userdata = info;

  ELF_sh_foreach(info->name, cb_list);

  if (!need_dbgsym) {
    info->debug_elf_path = strdup(info->name);
    return;
  }

  uint8_t *id = cb_list[1].userdata;
  char *path = malloc(512);
  int len = sprintf(path, "/usr/lib/debug/.build-id/%02x/", id[0]);
  int i;
  for (i = 1; i < 20; i ++) {
    len += sprintf(path + len, "%02x", id[i]);
  }
  strcat(path, ".debug");
  free(id);
  info->debug_elf_path = path;
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

void* get_loaded_addr(char *sym, int type) {
  return (void *)get_sym_addr(sym, type) + elf_base;
}

static void hack_entry() {
  void *addr = get_loaded_addr("main_loop_wait", STT_FUNC);
  mprotect_page((uintptr_t)addr, PROT_READ | PROT_EXEC);
  extern void difftest_init_late();
  difftest_init_late();
}

static int callback(struct dl_phdr_info *info, size_t size, void *data) {
  Info *arg = data;
  if (strcmp(info->dlpi_name, arg->name) == 0) {
    arg->base = info->dlpi_addr;
    return 1;
  }

  int i;
  for (i = 0; i < info->dlpi_phnum; i ++) {
    if (info->dlpi_phdr[i].p_type == PT_TLS) {
      assert(info->dlpi_tls_modid == arg->next_tls_modid);
      arg->tls_offset_diff += ALIGN_UP(info->dlpi_phdr[i].p_memsz, info->dlpi_phdr[i].p_align);
      arg->next_tls_modid ++;
      break;
    }
  }
  return 0;
}

static void hack_prepare(Info *info) {
  parse_origin_elf(info);
  if (access(info->debug_elf_path, R_OK) != 0) {
    printf("File '%s' does not exist!\n", info->debug_elf_path);
    printf("Make sure you are using QEMU installed by apt-get, "
           "and you have already installed the debug symbol package for qemu\n");
    assert(0);
  }

  parse_debug_elf(info->debug_elf_path);
  free(info->debug_elf_path);

  elf_base = info->base;

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
  mprotect_page((uintptr_t)qemu_main_loop_wait, PROT_READ | PROT_WRITE | PROT_EXEC);
  memcpy(qemu_main_loop_wait, &code, sizeof(code));
}

void dl_load(char *argv[]) {
  void *qemu = dlopen(argv[0], RTLD_LAZY);
  assert(qemu);
  int (*qemu_main)(int, char **, char **) = dlsym(qemu, "main");
  assert(qemu_main);

  Info info = { .name = argv[0], .next_tls_modid = 1, .tls_offset_diff = 0 };
  dl_iterate_phdr(callback, &info);

  hack_prepare(&info);

  char **p = argv;
  while (*p != NULL) p ++;
  int argc = p - argv;
  extern char **environ;
  qemu_main(argc, argv, environ);
  assert(0);
}
