#ifndef __RISCV64_H__
#define __RISCV64_H__

#define ISA_QEMU_BIN "qemu-system-riscv64"
#define ISA_QEMU_ARGS 

union isa_gdb_regs {
  struct {
    uint64_t gpr[32];
    uint64_t fpr[32];
    uint64_t pc;
  };
  struct {
    uint32_t array[DIFFTEST_REG_SIZE/sizeof(uint32_t)];
  };
};

#endif
