#include <common.h>
#include <elf.h>

#if defined(CONFIG_FTRACE_COND)
static char *strtab = NULL;
static Elf32_Sym *symtab = NULL;
static int nr_symtab_entry = 0;

void init_ftrace(const char *file) {
  FILE *fp = fopen(file, "rb");
  Assert(fp, "Can not open '%s'", file);

  uint8_t buf[sizeof(Elf32_Ehdr)];
  int ret = fread(buf, sizeof(Elf32_Ehdr), 1, fp);
  assert(ret == 1);

  /* The first several bytes contain the ELF header. */
  Elf32_Ehdr *elf = (void *)buf;
  char magic[] = {ELFMAG0, ELFMAG1, ELFMAG2, ELFMAG3};

  /* Check ELF header */
  assert(memcmp(elf->e_ident, magic, 4) == 0);		// magic number

  /* Load symbol table and string table for future use */

  /* Load section header table */
  uint32_t sh_size = elf->e_shentsize * elf->e_shnum;
  Elf32_Shdr *sh = malloc(sh_size);
  fseek(fp, elf->e_shoff, SEEK_SET);
  ret = fread(sh, sh_size, 1, fp);
  assert(ret == 1);

  /* Load section header string table */
  char *shstrtab = malloc(sh[elf->e_shstrndx].sh_size);
  fseek(fp, sh[elf->e_shstrndx].sh_offset, SEEK_SET);
  ret = fread(shstrtab, sh[elf->e_shstrndx].sh_size, 1, fp);
  assert(ret == 1);

  int i;
  for (i = 0; i < elf->e_shnum; i ++) {
    if (sh[i].sh_type == SHT_SYMTAB &&
        strcmp(shstrtab + sh[i].sh_name, ".symtab") == 0) {
      /* Load symbol table from file */
      symtab = malloc(sh[i].sh_size);
      fseek(fp, sh[i].sh_offset, SEEK_SET);
      ret = fread(symtab, sh[i].sh_size, 1, fp);
      assert(ret == 1);
      nr_symtab_entry = sh[i].sh_size / sizeof(symtab[0]);
    }
    else if (sh[i].sh_type == SHT_STRTAB &&
        strcmp(shstrtab + sh[i].sh_name, ".strtab") == 0) {
      /* Load string table from file */
      strtab = malloc(sh[i].sh_size);
      fseek(fp, sh[i].sh_offset, SEEK_SET);
      ret = fread(strtab, sh[i].sh_size, 1, fp);
      assert(ret == 1);
    }
  }

  free(sh);
  free(shstrtab);

  assert(strtab != NULL && symtab != NULL);

  fclose(fp);
}

static int depth = 0;

static const char* find_fun_name(word_t pc, bool exact) {
  static const char not_found[] = "???";

  int i;
  if (exact) {
    for (i = 0; i < nr_symtab_entry; i ++) {
      if (ELF32_ST_TYPE(symtab[i].st_info) == STT_FUNC && pc == symtab[i].st_value) {
        return strtab + symtab[i].st_name;
      }
    }
  } else {
    for (i = 0; i < nr_symtab_entry; i ++) {
      if (ELF32_ST_TYPE(symtab[i].st_info) == STT_FUNC &&
          pc >= symtab[i].st_value && pc < symtab[i].st_value + symtab[i].st_size) {
        return strtab + symtab[i].st_name;
      }
    }
  }

  return not_found;
}

void ftrace_call(word_t pc, word_t target) {
  if (FTRACE_COND) {
    log_write(FMT_WORD ": ", pc);
    int i;
    for (i = 0; i < depth; i ++) {
      log_write("  ");
    }
    log_write("call [%s@" FMT_WORD "]\n", find_fun_name(target, true), target);
    depth ++;
  }
}

void ftrace_ret(word_t pc) {
  if (FTRACE_COND) {
    depth --;
    log_write(FMT_WORD ": ", pc);
    int i;
    for (i = 0; i < depth; i ++) {
      log_write("  ");
    }
    log_write("ret  [%s]\n", find_fun_name(pc, false));
  }
}
#else
void init_ftrace(const char *file) { }
void ftrace_call(word_t pc, word_t target) { }
void ftrace_ret(word_t pc) { }
#endif
