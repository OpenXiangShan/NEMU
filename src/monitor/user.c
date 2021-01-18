#include <isa.h>
#include <memory/paddr.h>
#include <stdio.h>
#include <elf.h>

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
      void *host_addr = guest_to_host(ph->p_vaddr - PMEM_BASE);
      Log("loading to memory region [0x%x, 0x%x)", ph->p_vaddr, ph->p_vaddr + ph->p_memsz);
      fseek(fp, ph->p_offset, SEEK_SET);
      ret = fread(host_addr, ph->p_filesz, 1, fp);
      assert(ret == 1);
      memset(host_addr + ph->p_filesz, 0, ph->p_memsz - ph->p_filesz);
      if (ph->p_vaddr + ph->p_memsz > brk) brk = ph->p_vaddr + ph->p_memsz;
    }
  }
  fclose(fp);
  brk -= PMEM_BASE;
  cpu.pc = elf->e_entry;
  return brk;
}

long init_user(char *elfpath) {
  return load_elf(elfpath);
}
