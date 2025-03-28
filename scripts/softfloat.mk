ifdef CONFIG_FPU_SOFT

SOFTFLOAT = resource/softfloat/build/softfloat.a

SOFTFLOAT_REPO_PATH = resource/softfloat/repo
ifeq ($(wildcard $(SOFTFLOAT_REPO_PATH)/COPYING.txt),)
  $(shell git clone --depth=1 https://github.com/ucb-bar/berkeley-softfloat-3 $(SOFTFLOAT_REPO_PATH) 1>&2)
endif

SOFTFLOAT_BUILD_PATH = $(abspath $(SOFTFLOAT_REPO_PATH)/build/Linux-x86_64-GCC)

INC_DIR += $(SOFTFLOAT_REPO_PATH)/source/include
INC_DIR += $(SOFTFLOAT_REPO_PATH)/source/$(SPECIALIZE_TYPE)
LIBS += $(SOFTFLOAT)

ifeq ($(ISA),riscv64)
SPECIALIZE_TYPE = RISCV
else
SPECIALIZE_TYPE = 8086-SSE
endif

ifdef CONFIG_SHARE
SOFTFLOAT_OPTS_DEFAULT = -DSOFTFLOAT_ROUND_ODD -DINLINE_LEVEL=5 \
  -DSOFTFLOAT_FAST_DIV32TO16 -DSOFTFLOAT_FAST_DIV64TO32
SOFTFLOAT_OPTS_OVERRIDE = SOFTFLOAT_OPTS="$(SOFTFLOAT_OPTS_DEFAULT) -fPIC"
endif

$(SOFTFLOAT):
	SPECIALIZE_TYPE=$(SPECIALIZE_TYPE) $(SOFTFLOAT_OPTS_OVERRIDE) $(MAKE) -s -C $(SOFTFLOAT_BUILD_PATH) all
	mkdir -p $(@D)
	ln -sf $(SOFTFLOAT_BUILD_PATH)/softfloat.a $@

endif

clean-softfloat:
ifdef CONFIG_FPU_SOFT
	$(MAKE) -s -C $(SOFTFLOAT_BUILD_PATH) clean
endif

.PHONY: clean-softfloat
	
