ifeq ($(wildcard $(NEMU_HOME)/configs/pa_defconfig),)
  $(error NEMU_HOME=$(NEMU_HOME) is not a NEMU repo)
endif

-include $(NEMU_HOME)/include/config/auto.conf
-include $(NEMU_HOME)/include/config/auto.conf.cmd

SRCS-y = $(shell find src/ -name "*.c" | grep -v "src/\(isa\|engine\|user\)")

remove_quote = $(patsubst "%",%,$(1))

ISA    ?= $(if $(CONFIG_ISA),$(call remove_quote,$(CONFIG_ISA)),x86)
CFLAGS += -D__ISA__=$(ISA) -D_ISA_H_=\"isa/$(ISA).h\"
SRCS-y += $(shell find src/isa/$(ISA) -name "*.c")

ENGINE ?= $(call remove_quote,$(CONFIG_ENGINE))
CFLAGS += -D__ENGINE_$(ENGINE)__
INC_DIR += $(NEMU_HOME)/src/engine/$(ENGINE)
SRCS-y += $(shell find src/engine/$(ENGINE) -name "*.c")

CC = $(call remove_quote,$(CONFIG_CC))
CFLAGS_BUILD += $(call remove_quote,$(CONFIG_CC_OPT))
CFLAGS_BUILD += $(if $(CONFIG_CC_LTO),-flto,)
CFLAGS_BUILD += $(if $(CONFIG_CC_DEBUG),-ggdb3,)
CFLAGS  += $(CFLAGS_BUILD)
LDFLAGS += $(CFLAGS_BUILD)

NAME  = nemu-$(ENGINE)

ifndef CONFIG_SHARE
LDFLAGS += -lSDL2 -lreadline -ldl
else
SHARE = 1
endif

SRCS-$(CONFIG_MODE_USER) += $(shell find src/user -name "*.c")
SRCS += $(SRCS-y)

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
