NAME  = qemu
SRCS  = $(shell find src/ -name "*.c")

SHARE = 1
CFLAGS += -DNEMU_HOME=\"$(NEMU_HOME)\" -DCONFIG_ISA_$(GUEST_ISA)
INC_PATH += $(NEMU_HOME)/include

include $(NEMU_HOME)/scripts/isa.mk
include $(NEMU_HOME)/scripts/build.mk
