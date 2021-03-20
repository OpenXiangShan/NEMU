ifeq ($(wildcard $(NEMU_HOME)/src/nemu-main.c),)
  $(error NEMU_HOME=$(NEMU_HOME) is not a NEMU repo)
endif

-include $(NEMU_HOME)/include/config/auto.conf
-include $(NEMU_HOME)/include/config/auto.conf.cmd

DIRS-y = src/cpu src/memory src/monitor src/utils

remove_quote = $(patsubst "%",%,$(1))

ISA    ?= $(if $(CONFIG_ISA),$(call remove_quote,$(CONFIG_ISA)),x86)
CFLAGS += -D__ISA__=$(ISA)
INC_DIR += $(NEMU_HOME)/src/isa/$(ISA)/include
DIRS-y += src/isa/$(ISA)

ENGINE ?= $(call remove_quote,$(CONFIG_ENGINE))
INC_DIR += $(NEMU_HOME)/src/engine/$(ENGINE)
DIRS-y += src/engine/$(ENGINE)

DIRS-$(CONFIG_MODE_USER) += src/user

SRCS-y += src/nemu-main.c
DIRS-$(CONFIG_DEVICE) += src/device/io
SRCS-$(CONFIG_DEVICE) += src/device/device.c src/device/alarm.c src/device/intr.c
SRCS-$(CONFIG_HAS_SERIAL) += src/device/serial.c
SRCS-$(CONFIG_HAS_TIMER) += src/device/timer.c
SRCS-$(CONFIG_HAS_KEYBOARD) += src/device/keyboard.c
SRCS-$(CONFIG_HAS_VGA) += src/device/vga.c
SRCS-$(CONFIG_HAS_AUDIO) += src/device/audio.c
SRCS-$(CONFIG_HAS_DISK) += src/device/disk.c
SRCS-$(CONFIG_HAS_SDCARD) += src/device/sdcard.c

SRCS-y += $(shell find $(DIRS-y) -name "*.c")

SRCS = $(SRCS-y)

CC = $(call remove_quote,$(CONFIG_CC))
CFLAGS_BUILD += $(call remove_quote,$(CONFIG_CC_OPT))
CFLAGS_BUILD += $(if $(CONFIG_CC_LTO),-flto,)
CFLAGS_BUILD += $(if $(CONFIG_CC_DEBUG),-ggdb3,)
CFLAGS_BUILD += $(if $(CONFIG_CC_ASAN),-fsanitize=address,)
CFLAGS  += $(CFLAGS_BUILD)
LDFLAGS += $(CFLAGS_BUILD)

NAME  = nemu-$(ENGINE)

ifndef CONFIG_SHARE
LDFLAGS += -lSDL2 -lreadline -ldl -pie
else
SHARE = 1
endif

ifeq ($(ENGINE),interpreter)
SOFTFLOAT = resource/softfloat/build/softfloat.a
INC_DIR += resource/softfloat/repo/source/include
INC_DIR += resource/softfloat/repo/source/RISCV
LIBS += $(SOFTFLOAT)
$(SOFTFLOAT):
	$(MAKE) -s -C resource/softfloat/

.PHONY: $(SOFTFLOAT)
endif

include $(NEMU_HOME)/scripts/git.mk
include $(NEMU_HOME)/scripts/config.mk
include $(NEMU_HOME)/scripts/isa.mk
include $(NEMU_HOME)/scripts/build.mk

ifdef CONFIG_DIFFTEST
DIFF_REF_PATH = $(NEMU_HOME)/$(call remove_quote,$(CONFIG_DIFFTEST_REF_PATH))
DIFF_REF_SO = $(DIFF_REF_PATH)/build/$(ISA)-$(call remove_quote,$(CONFIG_DIFFTEST_REF_NAME))-so
MKFLAGS = ISA=$(ISA) SHARE=1 ENGINE=interpreter

$(DIFF_REF_SO):
	$(MAKE) -s -C $(DIFF_REF_PATH) $(MKFLAGS)

.PHONY: $(DIFF_REF_SO)
endif

compile_git:
	$(call git_commit, "compile")
$(BINARY): compile_git

# Some convenient rules

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

clean-tools = $(dir $(shell find ./tools -name "Makefile"))
$(clean-tools):
	-@$(MAKE) -s -C $@ clean
clean-tools: $(clean-tools)
clean-all: clean distclean clean-tools

.PHONY: run gdb run-env clean-tools clean-all $(clean-tools)
