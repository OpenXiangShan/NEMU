#include <common.h>
#include <elf.h>
#include <time.h>

static int (*qemu_cpu_memory_rw_debug)(void *cpu, long addr, uint8_t *buf, int len, int is_write) = NULL;
static int (*qemu_gdb_write_register)(void *cpu, uint8_t *buf, int reg) = NULL;
static int (*qemu_gdb_read_register)(void *cpu, uint8_t *buf, int reg) = NULL;
static int (*qemu_cpu_single_step)(void *cpu, int enabled) = NULL;
static void *qemu_cpu = NULL;
static int qemu_sstep_flags = 0;

void difftest_memcpy_from_dut(paddr_t dest, void *src, size_t n) {
  int ret = qemu_cpu_memory_rw_debug(qemu_cpu, dest, src, n, true);
  assert(ret == 0);
}

void difftest_getregs(void *r) {
  assert(0);
}

void difftest_setregs(const void *r) {
  assert(0);
}

void difftest_exec(uint64_t n) {
  assert(0);
}

void difftest_init(int port) {
  extern void dl_load(char *argv[]);
  char *argv[] = {
    "/usr/bin/qemu-system-i386",
    "-nographic", "-S", "-s", "-serial", "none", "-monitor", "none",
    NULL
  };
  dl_load(argv);
  assert(0);
}

typedef struct {
  // the first two members from the source code of QEMU
  void *c_cpu;
  void *g_cpu;
} GDBState;

void* get_loaded_addr(char *sym, int type);

void difftest_init_late() {
  GDBState **qemu_gdbserver_state = get_loaded_addr("gdbserver_state", STT_OBJECT);
  qemu_cpu_memory_rw_debug = get_loaded_addr("cpu_memory_rw_debug", STT_FUNC);
  qemu_gdb_write_register = get_loaded_addr("gdb_write_register", STT_FUNC);
  qemu_gdb_read_register = get_loaded_addr("gdb_read_register", STT_FUNC);
  qemu_cpu_single_step = get_loaded_addr("cpu_single_step", STT_FUNC);
  void (*qemu_mutex_unlock_iothread)() = get_loaded_addr("qemu_mutex_unlock_iothread", STT_FUNC);
  int (*qemu_tcg_cpu_exec)(void *) = get_loaded_addr("cpu_exec", STT_FUNC);
  int *flags = get_loaded_addr("sstep_flags", STT_OBJECT);
  qemu_sstep_flags = *flags;

  assert(*qemu_gdbserver_state);
  qemu_cpu = (*qemu_gdbserver_state)->g_cpu;
  assert(qemu_cpu);
  printf("ok\n");

#if 1
  int len = 0x10000;
  uint8_t *buf = malloc(len);
  memset(buf, 0x40, len);
  difftest_memcpy_from_dut(0x4000, buf, len);

  uint32_t val = 0x00000000;
  qemu_gdb_write_register(qemu_cpu, (void *)&val, 10); // cs
  uint32_t pc = 0x00004000;
  int pc_idx = 8;
#else
  int len = 0x10000;
  uint32_t *buf = malloc(len * 4);
  int jjj = 0;
  for (; jjj < len; jjj ++) {
    buf[jjj] = 0x24840001; // addi a0,a0,$1
  }
  difftest_memcpy_from_dut(0x80000000, buf, len * 4);

  uint32_t pc = 0x80000000;
  int32_t pc_idx = 37;
#endif
  free(buf);

  qemu_gdb_write_register(qemu_cpu, (void *)&pc, pc_idx); // pc
  qemu_cpu_single_step(qemu_cpu, qemu_sstep_flags);
  qemu_mutex_unlock_iothread();


  clock_t t0 = clock();
  int iii = 0;
  while (1) {
  qemu_gdb_write_register(qemu_cpu, (void *)&pc, pc_idx);
  int i;
  for (i = 0; i < len; i ++) {
    qemu_tcg_cpu_exec(qemu_cpu);
//    uint32_t val = 0;
//    qemu_gdb_read_register(qemu_cpu, (void *)&val, pc_idx);
//    printf("eip = 0x%x\n", val);
//    assert(val == 0x4000 + i);
  }

  iii ++;
  if (iii == 100) break;
  }
  clock_t t1 = clock();

  uint64_t total = iii * len;
  printf("finish, freq = %lld instr/s\n", total * 1000000ull / (t1 - t0));
  while (1);
}
