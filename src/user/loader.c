#include <isa.h>
#include <stdio.h>
#include <elf.h>
#include <sys/auxv.h>
#include "user.h"

#ifdef __ISA64__
# define Elf_Ehdr Elf64_Ehdr
# define Elf_Phdr Elf64_Phdr
#else
# define Elf_Ehdr Elf32_Ehdr
# define Elf_Phdr Elf32_Phdr
#endif

#if defined(__ISA_x86__)
# define ELF_TYPE EM_386
#elif defined(__ISA_mipS32__)
# define ELF_TYPE EM_MIPS
#elif defined(__ISA_riscv32__) || defined(__ISA_riscv64__)
# define ELF_TYPE EM_RISCV
#else
# error Unsupported ISA
#endif

void isa_init_user(word_t sp);

user_state_t user_state = {};

static long load_elf(char *elfpath) {
  Assert(elfpath != NULL, "User program is not given");
  FILE *fp = fopen(elfpath, "rb");
  Assert(fp, "Can not open '%s'", elfpath);

  Elf_Ehdr *elf;
  Elf_Phdr *ph, *eph;

  uint8_t buf[512];
  int ret = fread(buf, 512, 1, fp);
  assert(ret == 1);
  elf = (void*)buf;
  assert(buf[0] == 0x7f && buf[1] == 'E' && buf[2] == 'L' && buf[3] == 'F');
  assert(elf->e_machine == ELF_TYPE);

  /* Load each program segment */
  ph = (void *)elf + elf->e_phoff;
  eph = ph + elf->e_phnum;
  vaddr_t brk = 0;
  for (; ph < eph; ph ++) {
    if (ph->p_type == PT_LOAD) {
      void *host_addr = user_to_host(ph->p_vaddr);
      Log("loading to memory region [0x%x, 0x%x)", ph->p_vaddr, ph->p_vaddr + ph->p_memsz);
      fseek(fp, ph->p_offset, SEEK_SET);
      ret = fread(host_addr, ph->p_filesz, 1, fp);
      assert(ret == 1);
      memset(host_addr + ph->p_filesz, 0, ph->p_memsz - ph->p_filesz);
      if (ph->p_vaddr + ph->p_memsz > brk) brk = ph->p_vaddr + ph->p_memsz;
    }
  }
  fclose(fp);
  user_state.brk = brk;
  user_state.program_brk = brk;
  cpu.pc = elf->e_entry;
  return brk - PMEM_BASE;
}

static inline word_t init_stack() {
  word_t *sp = guest_to_host(PMEM_SIZE);
#define push(data) (*(-- sp) = data)
#define push_auxv(type, data) { push(data); push(type); }

  push_auxv(AT_NULL, 0);
  //push_auxv(AT_HWCAP, 0xbfebfbff);
  push_auxv(AT_PAGESZ, 0x1000);
  push_auxv(AT_CLKTCK, getauxval(AT_CLKTCK));
  push_auxv(AT_BASE, 0);
  push_auxv(AT_FLAGS, 0);
  push_auxv(AT_UID, getauxval(AT_UID));
  push_auxv(AT_EUID, getauxval(AT_EUID));
  push_auxv(AT_GID, getauxval(AT_GID));
  push_auxv(AT_EGID, getauxval(AT_EGID));
  push_auxv(AT_SECURE, getauxval(AT_SECURE));
  //push_auxv(AT_HWCAP2, 0);
  push(0); // delimiter
  // no envp
  push(0); // delimiter
  // no argv
  push(0); // argc

  return PMEM_BASE + host_to_guest(sp);
}

long init_user(char *elfpath) {
  word_t sp = init_stack();
  isa_init_user(sp);
  return load_elf(elfpath);
}
