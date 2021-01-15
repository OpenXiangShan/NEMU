#include <common.h>
#include <elf.h>
#include <setjmp.h>
#include <difftest.h>

static void (*qemu_cpu_physical_memory_rw)(long addr, uint8_t *buf, int len, int is_write) = NULL;
static int (*qemu_gdb_write_register)(void *cpu, uint8_t *buf, int reg) = NULL;
static int (*qemu_gdb_read_register)(void *cpu, uint8_t *buf, int reg) = NULL;
static int (*qemu_cpu_exec)(void *) = NULL;
static void *qemu_cpu = NULL;

extern char *isa_qemu_argv[];
void isa_raise_intr(uint64_t NO);
void init_isa();
void dl_load(char *argv[]);

void difftest_memcpy(paddr_t addr, void *buf, size_t n, bool direction) {
#ifdef __ISA_mips32__
  // It seems that qemu-system-mips treat 0x80000000 as a virtual address.
  // We should do the subtraction to get the address
  // which qemu-system-mips considers physical.
  addr -= 0x80000000;
#endif
  int is_write = direction == DIFFTEST_TO_REF ? true : false;
  qemu_cpu_physical_memory_rw(addr, buf, n, is_write);
}

void difftest_regcpy(void *dut, bool direction) {
  int (*fn)(void *cpu, uint8_t *buf, int reg) =
    (direction == DIFFTEST_TO_REF ? qemu_gdb_write_register : qemu_gdb_read_register);
  int total_size = DIFFTEST_REG_SIZE;
  int i = 0;
  while (total_size > 0) {
    int reg_size = fn(qemu_cpu, dut, i);
    dut += reg_size;
    total_size -= reg_size;
    i ++;
  }
}

void difftest_raise_intr(uint64_t NO) {
  isa_raise_intr(NO);
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
  if (setjmp(jbuf) == 0) {
    // first path
    dl_load(isa_qemu_argv); // never return
  }
}

void* get_loaded_addr(char *sym, int type);

void difftest_init_late() {
  qemu_cpu_physical_memory_rw = get_loaded_addr("cpu_physical_memory_rw", STT_FUNC);
  qemu_gdb_write_register = get_loaded_addr("gdb_write_register", STT_FUNC);
  qemu_gdb_read_register = get_loaded_addr("gdb_read_register", STT_FUNC);
  qemu_cpu_exec = get_loaded_addr("cpu_exec", STT_FUNC);

  int (*qemu_cpu_single_step)(void *cpu, int enabled) = get_loaded_addr("cpu_single_step", STT_FUNC);
  void (*qemu_mutex_unlock_iothread)() = get_loaded_addr("qemu_mutex_unlock_iothread", STT_FUNC);
  void* (*qemu_get_cpu)(int) = get_loaded_addr("qemu_get_cpu", STT_FUNC);
  int qemu_sstep_flags = *(int *)get_loaded_addr("sstep_flags", STT_OBJECT);

  qemu_cpu = qemu_get_cpu(0);
  assert(qemu_cpu);
  qemu_cpu_single_step(qemu_cpu, qemu_sstep_flags);
  qemu_mutex_unlock_iothread();

  init_isa();

  longjmp(jbuf, 1);
}

void qemu_write_reg(void *val, int idx) {
  qemu_gdb_write_register(qemu_cpu, val, idx);
}

void* qemu_get_cpu() {
  return qemu_cpu;
}
