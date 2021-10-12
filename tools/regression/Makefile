PROJECTN_HOME = $(NEMU_HOME)/..
NANOS_HOME = $(PROJECTN_HOME)/nanos-lite
AM_KERNELS_HOME = $(PROJECTN_HOME)/am-kernels

AMTESTS = $(AM_KERNELS_HOME)/tests/am-tests
HELLO = $(AM_KERNELS_HOME)/kernels/hello
MICROBENCH = $(AM_KERNELS_HOME)/benchmarks/microbench
NEMU_ON_AM = $(AM_KERNELS_HOME)/kernels/nemu
FCEUX = $(PROJECTN_HOME)/fceux-am/

MAKE_NEMU := $(MAKE) -C $(NEMU_HOME)
MAKE_NANOS := $(MAKE) -C $(NANOS_HOME)

all:
	$(MAKE) run-x86-pa
	$(MAKE) run-mips32
	$(MAKE) run-riscv32
	$(MAKE) run-riscv64-pa

# reset to a non-AM config
reset-config:
	$(MAKE_NEMU) ARCH=native riscv64_defconfig

run-x86-pa: reset-config
	$(MAKE) -C $(NEMU_HOME) x86-ref_defconfig
	$(MAKE) -C $(NEMU_HOME)
	$(MAKE) -C $(NEMU_HOME) x86-pa-difftest_defconfig
	$(MAKE) -C $(MICROBENCH) ARCH=x86-nemu run mainargs=test
	#$(MAKE) -C $(FCEUX) ARCH=x86-nemu run mainargs=mario
	$(MAKE) -C $(NANOS_HOME) ARCH=x86-nemu update VME=1
	$(MAKE) -C $(NANOS_HOME) ARCH=x86-nemu run

run-x86-user: reset-config
	$(MAKE) -C $(NEMU_HOME) x86-user-ref_defconfig
	$(MAKE) -C $(NEMU_HOME)
	$(MAKE) -C $(NEMU_HOME) x86-user_defconfig
	$(MAKE) -C ./tests/x86-syscall
	$(MAKE) -C $(NEMU_HOME) run ARGS=-b IMG=$(abspath ./tests/x86-syscall/build/x86-syscall.elf)
	$(MAKE) -C ./tests/hello ISA=x86
	$(MAKE) -C $(NEMU_HOME) run ARGS=-b IMG=$(abspath ./tests/hello/build/hello-x86.elf)
	$(MAKE) -C ./tests/float-test ISA=x86
	$(MAKE) -C $(NEMU_HOME) run ARGS=-b IMG=$(abspath ./tests/float-test/build/float-test-x86.elf)
	LOADER="$(NEMU_HOME)/build/x86-nemu-interpreter -b" $(MAKE) -C ~/sdi/spec2006-lite-i386/401.bzip2 run-test

run-mips32: reset-config
	$(MAKE) -C $(NEMU_HOME) mips32-difftest_defconfig
	$(MAKE) -C $(MICROBENCH) ARCH=mips32-nemu run mainargs=test
	$(MAKE) -C $(NANOS_HOME) ARCH=mips32-nemu update VME=1
	$(MAKE) -C $(NANOS_HOME) ARCH=mips32-nemu run

run-riscv32: reset-config
	$(MAKE) -C $(NEMU_HOME) riscv32-difftest_defconfig
	$(MAKE) -C $(MICROBENCH) ARCH=riscv32-nemu run mainargs=test
	$(MAKE) -C $(NANOS_HOME) ARCH=riscv32-nemu update VME=1
	$(MAKE) -C $(NANOS_HOME) ARCH=riscv32-nemu run

run-riscv64-pa: reset-config
	$(MAKE) -C $(NEMU_HOME) riscv64-pa-difftest_defconfig
	$(MAKE) -C $(MICROBENCH) ARCH=riscv64-nemu run mainargs=test
	$(MAKE) -C $(NANOS_HOME) ARCH=riscv64-nemu update VME=1
	$(MAKE) -C $(NANOS_HOME) ARCH=riscv64-nemu run

run-riscv64-user: reset-config
	$(MAKE) -C $(NEMU_HOME) riscv64-user_defconfig
	$(MAKE) -C ./tests/hello ISA=riscv64
	$(MAKE) -C $(NEMU_HOME) run ARGS=-b IMG=$(abspath ./tests/hello/build/hello-riscv64.elf)
	LOADER="$(NEMU_HOME)/build/riscv64-nemu-interpreter -b" $(MAKE) -C ~/projectn/spec2006-lite-riscv64/401.bzip2 run-test

TEST = $(MICROBENCH)

run-icount: reset-config
	$(MAKE) -C $(NEMU_HOME) riscv64-pa-icount-bb_defconfig
	$(MAKE) -C $(TEST) ARCH=riscv64-nemu run mainargs=ref
	$(MAKE) -C $(NEMU_HOME) riscv64-pa-icount-instr_defconfig
	$(MAKE) -C $(TEST) ARCH=riscv64-nemu run mainargs=ref
	$(MAKE) -C $(NEMU_HOME) riscv64-pa-non-perfopt_defconfig
	$(MAKE) -C $(TEST) ARCH=riscv64-nemu run mainargs=ref

run-nemu-on-am: reset-config
	$(MAKE) -C $(NEMU_HOME) riscv64-pa-difftest_defconfig
	$(MAKE) -C $(MICROBENCH) ARCH=riscv64-nemu mainargs=test
	$(MAKE) -C $(NEMU_ON_AM) ARCH=riscv64-nemu mainargs=$(MICROBENCH)/build/microbench-riscv64-nemu.bin

LINUX = $(PROJECTN_HOME)/workbench/build/linux.bin

run-linux: reset-config
	$(MAKE) -C $(NEMU_HOME) riscv64_defconfig
	$(MAKE) -C $(NEMU_HOME) run ARGS="-b -l build/nemu-log.txt" IMG=$(LINUX)
