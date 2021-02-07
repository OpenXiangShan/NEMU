NAME  = qemu
SRCS  = $(shell find src/ -name "*.c" | grep -v "isa")
SRCS += $(shell find src/isa/$(ISA) -name "*.c")

SHARE = 1
CFLAGS += -DNEMU_HOME=$(NEMU_HOME)

include $(NEMU_HOME)/scripts/isa.mk
include $(NEMU_HOME)/scripts/build.mk
