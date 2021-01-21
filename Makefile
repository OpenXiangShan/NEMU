ENGINE ?= interpreter
ENGINES = $(shell ls $(NEMU_HOME)/src/engine/)
ifeq ($(filter $(ENGINES), $(ENGINE)), ) # ENGINE must be valid
$(error Invalid ENGINE=$(ENGINE). Supported: $(ENGINES))
endif

NAME  = nemu-$(ENGINE)
SRCS  = $(shell find src/ -name "*.c" | grep -v "isa\|engine")
SRCS += $(shell find src/isa/$(ISA) -name "*.c")
SRCS += $(shell find src/engine/$(ENGINE) -name "*.c")

CFLAGS  += -ggdb3 -D__ENGINE_$(ENGINE)__ \
					 -D__ISA__=$(ISA) -D_ISA_H_=\"isa/$(ISA).h\"
INC_DIR += $(NEMU_HOME)/src/engine/$(ENGINE)
ifndef SHARE
LDFLAGS += -lSDL2 -lreadline -ldl
endif

include $(NEMU_HOME)/scripts/Makefile

ifndef SHARE
DIFF ?= qemu-dl
ifneq ($(ISA),x86)
ifeq ($(DIFF),kvm)
DIFF = qemu-dl
$(info KVM is only supported with ISA=x86, use QEMU instead)
endif
endif

ifeq ($(DIFF),qemu-socket)
DIFF_REF_PATH = $(NEMU_HOME)/tools/qemu-socket-diff
DIFF_REF_SO = $(DIFF_REF_PATH)/build/$(ISA)-qemu-so
CFLAGS += -D__DIFF_REF_QEMU_SOCKET__ -D__DIFF_REF_QEMU__
else ifeq ($(DIFF),qemu-dl)
DIFF_REF_PATH = $(NEMU_HOME)/tools/qemu-dl-diff
DIFF_REF_SO = $(DIFF_REF_PATH)/build/$(ISA)-qemu-so
CFLAGS += -D__DIFF_REF_QEMU_DL__ -D__DIFF_REF_QEMU__
else ifeq ($(DIFF),kvm)
DIFF_REF_PATH = $(NEMU_HOME)/tools/kvm-diff
DIFF_REF_SO = $(DIFF_REF_PATH)/build/$(ISA)-kvm-so
CFLAGS += -D__DIFF_REF_KVM__
else ifeq ($(DIFF),nemu)
DIFF_REF_PATH = $(NEMU_HOME)
DIFF_REF_SO = $(DIFF_REF_PATH)/build/$(ISA)-nemu-interpreter-so
CFLAGS += -D__DIFF_REF_NEMU__
MKFLAGS = ISA=$(ISA) SHARE=1 ENGINE=interpreter
else
$(error invalid DIFF. Supported: qemu-dl kvm qemu-socket nemu)
endif

$(DIFF_REF_SO):
	$(MAKE) -C $(DIFF_REF_PATH) $(MKFLAGS)

endif

include $(NEMU_HOME)/scripts/Makefile.git
compile_git:
	$(call git_commit, "compile")
$(BINARY): compile_git

# Some convenient rules

.PHONY: run gdb run-env $(DIFF_REF_SO)

override ARGS ?= --log=$(BUILD_DIR)/nemu-log.txt
override ARGS += --diff=$(DIFF_REF_SO)

# Command to execute NEMU
IMG ?=
NEMU_EXEC := $(BINARY) $(ARGS) $(IMG)

run-env: $(BINARY) $(DIFF_REF_SO)

run: run-env
	$(call git_commit, "run")
	$(NEMU_EXEC)

gdb: run-env
	$(call git_commit, "gdb")
	gdb -s $(BINARY) --args $(NEMU_EXEC)

clean: clean-tools
clean-tools:
	$(MAKE) -C tools/gen-expr clean
	$(MAKE) -C $(DIFF_REF_PATH) clean
