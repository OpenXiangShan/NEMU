#include <common.h>
#include <elf.h>
#include <setjmp.h>

static int (*qemu_cpu_physical_memory_rw)(long addr, uint8_t *buf, int len, int is_write) = NULL;
static int (*qemu_gdb_write_register)(void *cpu, uint8_t *buf, int reg) = NULL;
static int (*qemu_gdb_read_register)(void *cpu, uint8_t *buf, int reg) = NULL;
static int (*qemu_cpu_exec)(void *) = NULL;
static void (*qemu_do_interrupt_all)(void *cpu, int intno,
    int is_int, int error_code, uint32_t next_eip, int is_hw) = NULL;
static void *qemu_cpu = NULL;

void difftest_memcpy(paddr_t addr, void *buf, size_t n, bool to_ref) {
  int ret = qemu_cpu_physical_memory_rw(addr, buf, n, to_ref);
  assert(ret == 0);
}

void difftest_regcpy(void *dut, bool to_ref) {
  uint32_t *regs = dut;
  int (*fn)(void *cpu, uint8_t *buf, int reg) =
    (to_ref ? qemu_gdb_write_register : qemu_gdb_read_register);
  for (int i = 0; i < 9; i ++) { fn(qemu_cpu, (void *)&regs[i], i); }
}

void difftest_raise_intr(uint64_t NO) {
  qemu_do_interrupt_all(qemu_cpu, NO, 0, 0, 0, 1);
}

#define EXCP_INTERRUPT 0x10000
#define EXCP_HLT       0x10001
#define EXCP_DEBUG     0x10002
#define EXCP_ATOMIC    0x10005
void difftest_exec(uint64_t n) {
  for (; n > 0; n --) {
    int ret = qemu_cpu_exec(qemu_cpu);
    switch (ret) {
      case EXCP_ATOMIC:
      case EXCP_INTERRUPT: n ++; // fall through
      case EXCP_HLT:
      case EXCP_DEBUG: break;
      default: assert(0);
    }
  }
}

static jmp_buf jbuf = {};

void difftest_init(int port) {
  extern void dl_load(char *argv[]);
  if (setjmp(jbuf) == 0) {
    // first path
    char *argv[] = {
      "/usr/bin/qemu-system-i386",
//      "/home/yzh/software/qemu-v3.1.0/i386-softmmu/qemu-system-i386",
      "-nographic", "-S", "-serial", "none", "-monitor", "none",
      "-cpu", "Broadwell",
      NULL
    };
    dl_load(argv);
  }
}

void* get_loaded_addr(char *sym, int type);

void difftest_init_late() {
  qemu_cpu_physical_memory_rw = get_loaded_addr("cpu_physical_memory_rw", STT_FUNC);
  qemu_gdb_write_register = get_loaded_addr("gdb_write_register", STT_FUNC);
  qemu_gdb_read_register = get_loaded_addr("gdb_read_register", STT_FUNC);
  qemu_cpu_exec = get_loaded_addr("cpu_exec", STT_FUNC);
  qemu_do_interrupt_all = get_loaded_addr("do_interrupt_all", STT_FUNC);

  int (*qemu_cpu_single_step)(void *cpu, int enabled) = get_loaded_addr("cpu_single_step", STT_FUNC);
  void (*qemu_mutex_unlock_iothread)() = get_loaded_addr("qemu_mutex_unlock_iothread", STT_FUNC);
  void* (*qemu_get_cpu)(int) = get_loaded_addr("qemu_get_cpu", STT_FUNC);
  int qemu_sstep_flags = *(int *)get_loaded_addr("sstep_flags", STT_OBJECT);

  qemu_cpu = qemu_get_cpu(0);
  assert(qemu_cpu);
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
  difftest_memcpy(0x7c00, mbr, sizeof(mbr), true);

  // set cs:eip to 0000:7c00
  uint32_t val = 0;
  qemu_gdb_write_register(qemu_cpu, (void *)&val, 10); // cs
  val = 0x7c00;
  qemu_gdb_write_register(qemu_cpu, (void *)&val, 8); // pc

  // execute enough instructions to enter protected mode
  difftest_exec(20);

  longjmp(jbuf, 1);
}
