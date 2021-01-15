#include <common.h>

void difftest_memcpy(paddr_t addr, void *buf, size_t n, bool direction);
void difftest_exec(uint64_t n);
void qemu_write_reg(void *val, int idx);

char *isa_qemu_argv[] = {
  "/usr/bin/qemu-system-riscv32",
  "-nographic", "-S", "-serial", "none", "-monitor", "none",
  NULL
};

void init_isa() {
  uint32_t initcode[] = {
    0x800006b7,  // lui a3,0x8000
    0x03c68693,  // addi a3,a3,0x3c # 800003c
    0x30569073,  // csrw mtvec,a3

    0xfff00513,  // li a0, -1
    0x01f00593,  // li a1, 31
    0x3b051073,  // csrw pmpaddr0, a0
    0x3a059073,  // csrw pmpcfg0, a1

    0x000c1637,  // lui a2,0xc2       # c0001
    0x80060613,  // addi a2,a2,-2048  # c0800
    0x30061073,  // csrw mstatus,a2

    0x00468693,  // addi a3,a3,0x10 # 8000040
    0x34169073,  // csrw mepc,a3

    0x30251073,  // csrw medeleg, a0

    0x30200073,  // mret

    // here:
    0x0000006f,  // j here # spin
    0x0000006f,  // # spin
    0x0000006f,  // # spin
    0x0000006f,  // # spin
  };

  // put initcode to QEMU to setup a PMP to permit access to all of memory in S mode
  difftest_memcpy(0x80000000, initcode, sizeof(initcode), true);

  // set pc to 0x80000000
  uint32_t val = 0x80000000;
  qemu_write_reg((void *)&val, 32); // pc

  // execute enough instructions to enter protected mode
  difftest_exec(20);
}
