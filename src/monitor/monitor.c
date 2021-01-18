#include <isa.h>
#include <memory/paddr.h>
#include <monitor/monitor.h>
#include <getopt.h>
#include <stdlib.h>
#include <elf.h>

void init_log(const char *log_file);
void init_mem();
void init_regex();
void init_wp_pool();
void init_difftest(char *ref_so_file, long img_size, int port);

static char *log_file = NULL;
static char *diff_so_file = NULL;
static char *img_file = NULL;
static int batch_mode = false;
static int difftest_port = 1234;

int is_batch_mode() { return batch_mode; }

static inline void welcome() {
#ifdef DEBUG
  Log("Debug: \33[1;32m%s\33[0m", "ON");
  Log("If debug mode is on, A log file will be generated to record every instruction NEMU executes. "
      "This may lead to a large log file. "
      "If it is not necessary, you can turn it off in include/common.h.");
#else
  Log("Debug: \33[1;32m%s\33[0m", "OFF");
#endif

  Log("Build time: %s, %s", __TIME__, __DATE__);
  printf("Welcome to \33[1;41m\33[1;33m%s\33[0m-NEMU!\n", str(__ISA__));
  printf("For help, type \"help\"\n");
}

static inline long load_img() {
  if (img_file == NULL) {
    Log("No image is given. Use the default build-in image.");
    return 4096; // built-in image size
  }

  FILE *fp = fopen(img_file, "rb");
  Assert(fp, "Can not open '%s'", img_file);

  Log("The image is %s", img_file);

  long size = 0;

#ifdef USER_MODE

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
  brk -= PMEM_BASE;
  size = brk;

  cpu.pc = elf->e_entry;
  Log("cpu.pc = 0x%x", cpu.pc);
#else
  fseek(fp, 0, SEEK_END);
  size = ftell(fp);

  fseek(fp, 0, SEEK_SET);
  int ret = fread(guest_to_host(IMAGE_START), size, 1, fp);
  assert(ret == 1);
#endif

  fclose(fp);
  return size;
}

static inline void parse_args(int argc, char *argv[]) {
  const struct option table[] = {
    {"batch"    , no_argument      , NULL, 'b'},
    {"log"      , required_argument, NULL, 'l'},
    {"diff"     , required_argument, NULL, 'd'},
    {"port"     , required_argument, NULL, 'p'},
    {"help"     , no_argument      , NULL, 'h'},
    {0          , 0                , NULL,  0 },
  };
  int o;
  while ( (o = getopt_long(argc, argv, "-bhl:d:p:", table, NULL)) != -1) {
    switch (o) {
      case 'b': batch_mode = true; break;
      case 'p': sscanf(optarg, "%d", &difftest_port); break;
      case 'l': log_file = optarg; break;
      case 'd': diff_so_file = optarg; break;
      case 1:
        if (img_file != NULL) Log("too much argument '%s', ignored", optarg);
        else img_file = optarg;
        break;
      default:
        printf("Usage: %s [OPTION...] IMAGE\n\n", argv[0]);
        printf("\t-b,--batch              run with batch mode\n");
        printf("\t-l,--log=FILE           output log to FILE\n");
        printf("\t-d,--diff=REF_SO        run DiffTest with reference REF_SO\n");
        printf("\t-p,--port=PORT          run DiffTest with port PORT\n");
        printf("\n");
        exit(0);
    }
  }
}

void init_monitor(int argc, char *argv[]) {
  /* Perform some global initialization. */

  /* Parse arguments. */
  parse_args(argc, argv);

  /* Open the log file. */
  init_log(log_file);

  /* Fill the memory with garbage content. */
  init_mem();

  /* Perform ISA dependent initialization. */
  init_isa();

  /* Load the image to memory. This will overwrite the built-in image. */
  long img_size = load_img();

  /* Compile the regular expressions. */
  init_regex();

  /* Initialize the watchpoint pool. */
  init_wp_pool();

  /* Initialize differential testing. */
  init_difftest(diff_so_file, img_size, difftest_port);

  /* Display welcome message. */
  welcome();
}
