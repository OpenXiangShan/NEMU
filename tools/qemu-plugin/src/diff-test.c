#include <common.h>
#include <elf.h>
#include <setjmp.h>
#include <time.h>

static int (*qemu_cpu_memory_rw_debug)(void *cpu, long addr, uint8_t *buf, int len, int is_write) = NULL;
static int (*qemu_gdb_write_register)(void *cpu, uint8_t *buf, int reg) = NULL;
static int (*qemu_gdb_read_register)(void *cpu, uint8_t *buf, int reg) = NULL;
static int (*qemu_cpu_exec)(void *) = NULL;
static void *qemu_cpu = NULL;

void difftest_memcpy_from_dut(paddr_t dest, void *src, size_t n) {
  int ret = qemu_cpu_memory_rw_debug(qemu_cpu, dest, src, n, true);
  assert(ret == 0);
}

void difftest_getregs(void *r) {
  uint32_t *regs = r;
  int i;
  for (i = 0; i < 9; i ++) {
    qemu_gdb_read_register(qemu_cpu, (void *)&regs[i], i);
  }
}

void difftest_setregs(const void *r) {
  uint32_t *regs = (void *)r;
  int i;
  for (i = 0; i < 9; i ++) {
    qemu_gdb_write_register(qemu_cpu, (void *)&regs[i], i);
  }
}

void difftest_exec(uint64_t n) {
  for (; n > 0; n --) qemu_cpu_exec(qemu_cpu);
}

static jmp_buf jbuf = {};

void difftest_init(int port) {
  extern void dl_load(char *argv[]);
  if (setjmp(jbuf) == 0) {
    // first path
    char *argv[] = {
//      "/usr/bin/qemu-system-i386",
      "/home/yzh/software/qemu-v3.1.0/i386-softmmu/qemu-system-i386",
      "-nographic", "-S", "-s", "-serial", "none", "-monitor", "none",
      NULL
    };
    dl_load(argv);
  }
}

typedef struct {
  // the first two members from the source code of QEMU
  void *c_cpu;
  void *g_cpu;
} GDBState;

void* get_loaded_addr(char *sym, int type);

void difftest_init_late() {
  qemu_cpu_memory_rw_debug = get_loaded_addr("cpu_memory_rw_debug", STT_FUNC);
  qemu_gdb_write_register = get_loaded_addr("gdb_write_register", STT_FUNC);
  qemu_gdb_read_register = get_loaded_addr("gdb_read_register", STT_FUNC);
  qemu_cpu_exec = get_loaded_addr("cpu_exec", STT_FUNC);

  int (*qemu_cpu_single_step)(void *cpu, int enabled) = get_loaded_addr("cpu_single_step", STT_FUNC);
  void (*qemu_mutex_unlock_iothread)() = get_loaded_addr("qemu_mutex_unlock_iothread", STT_FUNC);
  int qemu_sstep_flags = *(int *)get_loaded_addr("sstep_flags", STT_OBJECT);
  GDBState **qemu_gdbserver_state = get_loaded_addr("gdbserver_state", STT_OBJECT);

  printf("qemu_gdbserver_state = %p\n", qemu_gdbserver_state);
  assert(*qemu_gdbserver_state);
  qemu_cpu = (*qemu_gdbserver_state)->g_cpu;
  assert(qemu_cpu);
  printf("ok\n");

  qemu_cpu_single_step(qemu_cpu, qemu_sstep_flags);
  qemu_mutex_unlock_iothread();

  static uint8_t mbr[] = {
    // start16:
    0xfa,                           // cli
    0x31, 0xc0,                     // xorw   %ax,%ax
    0x8e, 0xd8,                     // movw   %ax,%ds
    0x8e, 0xc0,                     // movw   %ax,%es
    0x8e, 0xd0,                     // movw   %ax,%ss
    0x0f, 0x01, 0x16, 0x44, 0x7c,   // lgdt   gdtdesc
    0x0f, 0x20, 0xc0,               // movl   %cr0,%eax
    0x66, 0x83, 0xc8, 0x01,         // orl    $CR0_PE,%eax
    0x0f, 0x22, 0xc0,               // movl   %eax,%cr0
    0xea, 0x1d, 0x7c, 0x08, 0x00,   // ljmp   $GDT_ENTRY(1),$start32

    // start32:
    0x66, 0xb8, 0x10, 0x00,         // movw   $0x10,%ax
    0x8e, 0xd8,                     // movw   %ax, %ds
    0x8e, 0xc0,                     // movw   %ax, %es
    0x8e, 0xd0,                     // movw   %ax, %ss
    0xeb, 0xfe,                     // jmp    7c27
    0x8d, 0x76, 0x00,               // lea    0x0(%esi),%esi

    // GDT
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xff, 0xff, 0x00, 0x00, 0x00, 0x9a, 0xcf, 0x00,
    0xff, 0xff, 0x00, 0x00, 0x00, 0x92, 0xcf, 0x00,

    // GDT descriptor
    0x17, 0x00, 0x2c, 0x7c, 0x00, 0x00
  };

  // put the MBR code to QEMU to enable protected mode
  difftest_memcpy_from_dut(0x7c00, mbr, sizeof(mbr));

  // set cs:eip to 0000:7c00
  uint32_t val = 0;
  qemu_gdb_write_register(qemu_cpu, (void *)&val, 10); // cs
  val = 0x7c00;
  qemu_gdb_write_register(qemu_cpu, (void *)&val, 8); // cs

  // execute enough instructions to enter protected mode
  difftest_exec(20);

  longjmp(jbuf, 1);
}
