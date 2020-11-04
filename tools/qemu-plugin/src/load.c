#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <alloca.h>
#include <elf.h>
#include <sys/mman.h>
#include <sys/auxv.h>
#include <stdlib.h>

typedef struct {
  uintptr_t base;
  uintptr_t entry;
  uintptr_t phdr;
  int phent;
  int phnum;
} ELFinfo;

static void loadELF(const char *filename, ELFinfo *info) {
  FILE *fp = fopen(filename, "r");
  assert(fp);
  void *buf = malloc(4096);
  assert(buf);
  int ret = fread(buf, 4096, 1, fp);
  assert(ret == 1);

  Elf64_Ehdr *elf = buf;
  int i;
  uintptr_t end = 0;
  Elf64_Phdr *ph = buf + elf->e_phoff;
  for (i = 0; i < elf->e_phnum; i ++) {
    if (ph[i].p_type == PT_LOAD) {
      uintptr_t seg_end = ph[i].p_vaddr + ph[i].p_memsz;
      if (end < seg_end) end = seg_end;
    }
  }

  void *p = mmap(NULL, end, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  assert(p != (void *)-1);

  for (i = 0; i < elf->e_phnum; i ++) {
    if (ph[i].p_type == PT_LOAD) {
      uintptr_t va     = ph[i].p_vaddr;
      uintptr_t offset = ph[i].p_offset;
      uintptr_t filesz = ph[i].p_filesz;
      uintptr_t memsz  = ph[i].p_memsz;
      uintptr_t pad = va & 0xfff;
      if (pad != 0) {
        va -= pad;
        offset -= pad;
        filesz += pad;
        memsz += pad;
      }
      int prot = PROT_READ;
      if (ph[i].p_flags & PF_W) prot |= PROT_WRITE;
      if (ph[i].p_flags & PF_X) prot |= PROT_EXEC;
      printf("mapping [%p,%p) with prot = R%c%c\n", p + va, p + va + filesz,
          (prot & PROT_WRITE ? 'W' : '-'), (prot & PROT_EXEC ? 'X' : '-'));
      void *ret = mmap(p + va, filesz, prot, MAP_PRIVATE | MAP_FIXED, fileno(fp), offset);
      assert(ret != (void *)-1);
      if (memsz != filesz) {
        pad = ((filesz + 0xfff) & ~0xfff) - filesz;
        printf("pad = %ld\n", pad);
        if (pad > 0) memset(p + va + filesz, 0, pad);
      }
    }
  }

  fclose(fp);

  info->base = (uintptr_t)p;
  info->entry = info->base + elf->e_entry;
  info->phdr = info->base + elf->e_phoff;
  info->phent = elf->e_phentsize;
  info->phnum = elf->e_phnum;

  free(buf);
}

typedef struct {
  int a_type;
  union {
    long a_val;
    void *a_ptr;
    void (*a_fnc)();
  } a_un;
} auxv_t;

void dl_load(char *argv[]) {
  char **p = argv;
  while (*p != NULL) p ++;
  int argc = p - argv;

  argc ++; // add ld.so
  int argv_size = argc * sizeof(argv[0]);
  char **ld_argv = malloc(argv_size);
  ld_argv[0] = "/lib64/ld-linux-x86-64.so.2";
  memcpy(&ld_argv[1], argv, argv_size);

  ELFinfo ld_elf, bin_elf;
  loadELF(ld_argv[0], &ld_elf);
  loadELF(ld_argv[1], &bin_elf);

  extern char **environ;
  p = environ;
  while (*p != NULL) p ++;
  p ++; // count the NULL
  int env_size = (void *)p - (void *)environ;

  auxv_t *aux_start = (void *)p;
  auxv_t *aux = aux_start;
  while (aux->a_type != AT_NULL) aux ++;
  aux ++; //count the NULL
  int aux_size = (void *)aux - (void *)p;

  int size = sizeof(uintptr_t) + argv_size + env_size + aux_size;
  uintptr_t *rsp = alloca(size);
  rsp[0] = argc;
  p = (void *)(rsp + 1);
  memcpy(p, ld_argv, argv_size);
  p += argc;
  *p ++ = NULL;
  memcpy(p, environ, env_size);
  p += env_size / sizeof(char *);
  memcpy(p, aux_start, aux_size);

  aux = (void *)p;
  for (; aux->a_type != AT_NULL; aux ++) {
    switch (aux->a_type) {
      case AT_PHDR: aux->a_un.a_val = bin_elf.phdr; break;
      case AT_PHNUM: aux->a_un.a_val = bin_elf.phnum; break;
      case AT_BASE: aux->a_un.a_val = bin_elf.base; break;
      case AT_ENTRY: aux->a_un.a_val = bin_elf.entry; break;
      case AT_EXECFN: aux->a_un.a_ptr = (void *)ld_argv[1]; break;
    }
  }

  printf("entry = 0x%lx\n", bin_elf.entry);

  asm volatile ("mov %0, %%rsp; jmp *%1": : "r"(rsp), "r"(ld_elf.entry));
  assert(0);
}
