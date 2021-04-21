NAME  = kvm
SRCS  = $(shell find src/ -name "*.c")

SHARE = 1
INC_DIR += $(NEMU_HOME)/include $(NEMU_HOME)/src/isa/x86/include
ISA = x86

include $(NEMU_HOME)/scripts/isa.mk
include $(NEMU_HOME)/scripts/build.mk
