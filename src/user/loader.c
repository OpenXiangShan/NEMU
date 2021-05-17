#include <isa.h>
#include <stdio.h>
#include <elf.h>
#include <sys/auxv.h>
#include <unistd.h>
#include "user.h"

# define Elf_Ehdr MUXDEF(CONFIG_ISA64, Elf64_Ehdr, Elf32_Ehdr)
# define Elf_Phdr MUXDEF(CONFIG_ISA64, Elf64_Phdr, Elf32_Phdr)

#if defined(CONFIG_ISA_x86)
# define ELF_TYPE EM_386
#elif defined(CONFIG_ISA_mips32)
# define ELF_TYPE EM_MIPS
#elif defined(CONFIG_ISA_riscv32) || defined(CONFIG_ISA_riscv64)
# define ELF_TYPE EM_RISCV
#else
# error Unsupported ISA
#endif

void isa_init_user(word_t sp);

user_state_t user_state = {};

static void load_elf(char *elfpath) {
  Assert(elfpath != NULL, "User program is not given");
  FILE *fp = fopen(elfpath, "rb");
  Assert(fp, "Can not open '%s'", elfpath);

  Elf_Ehdr *elf;
  Elf_Phdr *ph, *eph;

  uint8_t buf[512];
  int ret = fread(buf, 512, 1, fp);
  assert(ret == 1);
  elf = (Elf_Ehdr *)buf;
  assert(buf[0] == 0x7f && buf[1] == 'E' && buf[2] == 'L' && buf[3] == 'F');
  assert(elf->e_machine == ELF_TYPE);

  /* Load each program segment */
  ph = (Elf_Phdr *)((uint8_t *)elf + elf->e_phoff);
  eph = ph + elf->e_phnum;
  vaddr_t brk = 0;
  for (; ph < eph; ph ++) {
    if (ph->p_type == PT_LOAD) {
      uint32_t pad_byte = ph->p_vaddr % PAGE_SIZE;
      ph->p_vaddr -= pad_byte;
      ph->p_offset -= pad_byte;
      ph->p_filesz += pad_byte;
      ph->p_memsz += pad_byte;

      uint8_t *haddr = user_to_host(ph->p_vaddr);
      if (ph->p_filesz != 0) {
        user_mmap(haddr, ph->p_filesz, PROT_READ | PROT_WRITE,
            MAP_PRIVATE | MAP_FIXED, fileno(fp), ph->p_offset);
      }
      if (ph->p_flags & PF_W) {
        // bss
        memset(haddr + ph->p_filesz, 0, PAGE_SIZE - ph->p_filesz % PAGE_SIZE);
        void *bss_page = haddr + ROUNDUP(ph->p_filesz, PAGE_SIZE);
        sword_t memsz = ph->p_memsz - ROUNDUP(ph->p_filesz, PAGE_SIZE);
        if (memsz > 0) {
          user_mmap(bss_page, memsz, PROT_READ | PROT_WRITE,
              MAP_PRIVATE | MAP_FIXED | MAP_ANONYMOUS, -1, 0);
        }
      }

      if (ph->p_vaddr + ph->p_memsz > brk) brk = ph->p_vaddr + ph->p_memsz;
      if (ph->p_offset == 0) { user_state.phdr = ph->p_vaddr + elf->e_phoff; }
    }
  }
  fclose(fp);
  user_state.brk = brk;
  user_state.brk_page = ROUNDUP(brk, PAGE_SIZE);
  user_state.program_brk = brk;
  user_state.entry = elf->e_entry;
  user_state.phent = elf->e_phentsize;
  user_state.phnum = elf->e_phnum;
  cpu.pc = elf->e_entry;
}

static inline word_t init_stack(int argc, char *argv[]) {
  uint8_t *sp = user_to_host(0xc0000000);
  uint32_t stack_size = 8 * 1024 * 1024;
  user_mmap(sp - stack_size, stack_size, PROT_READ | PROT_WRITE,
      MAP_PRIVATE | MAP_FIXED | MAP_ANONYMOUS, -1, 0);

  word_t strs[128] = {};
  int i = 0;
  char *envp[] = { NULL };
#define push(data) { sp -= sizeof(word_t); *(word_t *)sp = (data); }
#define push_auxv(type, data) { push(data); push(type); }
#define push_mem(src, size) { \
  sp -= (size); \
  memcpy(sp, (src), (size)); \
}
#define push_str(s) push_mem(s, strlen(s) + 1)
#define push_str_array(arr) { \
  char **p = arr; \
  for (; *p != NULL; p ++) { \
    push_str(*p); \
    assert(i < ARRLEN(strs)); \
    strs[i ++] = host_to_user(sp); \
  } \
  strs[i ++] = 0; \
}

  push_str_array(argv);
  push_str_array(envp);

  // aligning
  sp = (uint8_t *)ROUNDDOWN(sp, sizeof(word_t));

  // AT_RANDOM
  push(0xdeadbeef); push(0xdeadbeef); push(0xdeadbeef); push(0xdeadbeef);
  word_t random_ptr = host_to_user(sp);

  push_auxv(AT_NULL, 0);
  //push_auxv(AT_HWCAP, 0xbfebfbff);
  push_auxv(AT_PAGESZ, 0x1000);
  push_auxv(AT_CLKTCK, getauxval(AT_CLKTCK));
  push_auxv(AT_PHDR, user_state.phdr);
  push_auxv(AT_PHENT, user_state.phent);
  push_auxv(AT_PHNUM, user_state.phnum);
  push_auxv(AT_BASE, 0);
  push_auxv(AT_FLAGS, 0);
  push_auxv(AT_ENTRY, user_state.entry);
  push_auxv(AT_UID, getauxval(AT_UID));
  push_auxv(AT_EUID, getauxval(AT_EUID));
  push_auxv(AT_GID, getauxval(AT_GID));
  push_auxv(AT_EGID, getauxval(AT_EGID));
  push_auxv(AT_SECURE, getauxval(AT_SECURE));
  push_auxv(AT_RANDOM, random_ptr);
  //push_auxv(AT_HWCAP2, 0);

  push_mem(strs, sizeof(strs[0]) * i);
  push(argc);

  return host_to_user(sp);
}

static void redirction_std() {
  user_state.std_fd[0] = dup(0);
  user_state.std_fd[1] = dup(1);
  user_state.std_fd[2] = dup(2);
  FILE *fp;
  fp = freopen("/dev/tty", "r", stdin);  assert(fp);
  fp = freopen("/dev/tty", "w", stdout); assert(fp);
  fp = freopen("/dev/tty", "w", stdout); assert(fp);
}

void init_user(char *elfpath, int argc, char *argv[]) {
  redirction_std();
  load_elf(elfpath);
  word_t sp = init_stack(argc, argv);
  isa_init_user(sp);
}
