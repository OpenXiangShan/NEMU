#***************************************************************************************
# Copyright (c) 2014-2021 Zihao Yu, Nanjing University
# Copyright (c) 2020-2022 Institute of Computing Technology, Chinese Academy of Sciences
#
# NEMU is licensed under Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#          http://license.coscl.org.cn/MulanPSL2
#
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
# EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
# MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
#
# See the Mulan PSL v2 for more details.
#**************************************************************************************/

ifeq ($(wildcard $(NEMU_HOME)/src/nemu-main.c),)
  $(error NEMU_HOME=$(NEMU_HOME) is not a NEMU repo)
endif

-include $(NEMU_HOME)/include/config/auto.conf
-include $(NEMU_HOME)/include/config/auto.conf.cmd

DIRS-y = src/cpu src/monitor src/utils
DIRS-$(CONFIG_MODE_SYSTEM) += src/memory

remove_quote = $(patsubst "%",%,$(1))

ISA    ?= $(if $(CONFIG_ISA),$(call remove_quote,$(CONFIG_ISA)),x86)
CFLAGS += -D__ISA__=$(ISA)

ifdef CONFIG_ENABLE_CONFIG_MMIO_SPACE
CFLAGS += -D__MMIO_SPECE_RANGE__=$(call remove_quote,$(CONFIG_MMIO_SPACE_RANGE))
endif

ifdef CONFIG_HAS_FLASH
FLASH_IMG_PATH=$(shell realpath $(CONFIG_FLASH_IMG_PATH) 2>/dev/null)
ifeq ($(FLASH_IMG_PATH),)
ifneq ($(CONFIG_FLASH_IMG_PATH),)
$(warning You have set CONFIG_FLASH_IMG_PATH option, but the specified path '$(CONFIG_FLASH_IMG_PATH)' does not exist!)
endif
CFLAGS += -D__FLASH_IMG_PATH__=\"\"
else
CFLAGS += -D__FLASH_IMG_PATH__=\"$(FLASH_IMG_PATH)\"
endif # ($(FLASH_IMG_PATH),)
endif # CONFIG_HAS_FLASH

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
SRCS-$(CONFIG_HAS_UARTLITE) += src/device/uartlite.c
SRCS-$(CONFIG_HAS_UART_SNPS) += src/device/uart_snps.c
SRCS-$(CONFIG_HAS_PLIC) += src/device/plic.c
SRCS-$(CONFIG_HAS_TIMER) += src/device/timer.c
SRCS-$(CONFIG_HAS_KEYBOARD) += src/device/keyboard.c
SRCS-$(CONFIG_HAS_VGA) += src/device/vga.c
SRCS-$(CONFIG_HAS_AUDIO) += src/device/audio.c
SRCS-$(CONFIG_HAS_DISK) += src/device/disk.c
SRCS-$(CONFIG_HAS_SDCARD) += src/device/sdcard.c
SRCS-$(CONFIG_HAS_FLASH) += src/device/flash.c

DIRS-y += src/profiling

ifndef CONFIG_SHARE
DIRS-y += src/checkpoint
endif

SRCS-y += $(shell find $(DIRS-y) -name "*.c")

SRCS = $(SRCS-y)

XSRCS-$(CONFIG_USE_SPARSEMM) += src/memory/sparseram.cpp

ifndef CONFIG_SHARE
XDIRS-y += src/checkpoint src/base src/iostream3 src/profiling
XSRCS-y += $(shell find $(XDIRS-y) -name "*.cpp")
endif

XSRCS-y += src/memory/store_queue_wrapper.cpp

XSRCS = $(XSRCS-y)

CC = $(call remove_quote,$(CONFIG_CC))
CXX = $(call remove_quote,$(CONFIG_CXX))
CFLAGS_BUILD += $(call remove_quote,$(CONFIG_CC_OPT))
CFLAGS_BUILD += $(if $(CONFIG_CC_LTO),-flto=auto,)
CFLAGS_BUILD += $(if $(CONFIG_CC_DEBUG),-ggdb3,)
CFLAGS_BUILD += $(if $(CONFIG_CC_ASAN),-fsanitize=address,)
CFLAGS  += $(CFLAGS_BUILD)
LDFLAGS += $(CFLAGS_BUILD)

NAME  = nemu-$(ENGINE)

ifndef CONFIG_SHARE
ifdef CONFIG_CC_NATIVE_ARCH
CFLAGS  += -march=native -mtune=native
CFLAGS	+= -ftree-vectorize # vector unit stride fast path
endif
LDFLAGS += -lz
endif

ifndef CONFIG_SHARE
LDFLAGS += -lreadline -ldl
else
SHARE = 1
endif

ifdef CONFIG_DEVICE
ifndef CONFIG_SHARE
LDFLAGS += -lSDL2
endif
endif

ifdef CONFIG_FPU_HOST
LDFLAGS += -lm
endif

include $(NEMU_HOME)/scripts/repos.mk
include $(NEMU_HOME)/scripts/git.mk
include $(NEMU_HOME)/scripts/config.mk
include $(NEMU_HOME)/scripts/isa.mk
include $(NEMU_HOME)/scripts/build.mk

ifdef CONFIG_DIFFTEST
DIFF_REF_PATH = $(NEMU_HOME)/$(call remove_quote,$(CONFIG_DIFFTEST_REF_PATH))
DIFF_REF_SO = $(DIFF_REF_PATH)/build/$(ISA)-$(call remove_quote,$(CONFIG_DIFFTEST_REF_NAME))-so
MKFLAGS = ISA=$(ISA) SHARE=1 ENGINE=interpreter
ARGS_DIFF = --diff=$(DIFF_REF_SO)

ifndef CONFIG_DIFFTEST_REF_NEMU
$(DIFF_REF_SO):
	$(MAKE) -s -C $(DIFF_REF_PATH) $(MKFLAGS)
endif

.PHONY: $(DIFF_REF_SO)
endif

compile_git:
	$(call git_commit, "compile")
$(BINARY): compile_git

# Some convenient rules

override ARGS ?= --log=$(BUILD_DIR)/nemu-log.txt
override ARGS += $(ARGS_DIFF)

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
clean-all: clean distclean clean-tools clean-repos

.PHONY: run gdb run-env clean-tools clean-all $(clean-tools)
