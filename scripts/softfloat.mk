
SOFTFLOAT_REPO_PATH = resource/softfloat/repo
ifeq ($(wildcard $(SOFTFLOAT_REPO_PATH)/COPYING.txt),)
  $(shell git clone --depth=1 https://github.com/ucb-bar/berkeley-softfloat-3 $(SOFTFLOAT_REPO_PATH))
endif

SOFTFLOAT_BUILD_PATH = $(abspath $(SOFTFLOAT_REPO_PATH)/build/Linux-x86_64-GCC)

INC_DIR += $(SOFTFLOAT_REPO_PATH)/source/include
INC_DIR += $(SOFTFLOAT_REPO_PATH)/source/$(SPECIALIZE_TYPE)
LIBS += $(SOFTFLOAT)

$(SOFTFLOAT):
	SPECIALIZE_TYPE=$(SPECIALIZE_TYPE) $(SOFTFLOAT_OPTS_OVERRIDE) $(MAKE) -s -C $(SOFTFLOAT_BUILD_PATH) all
	mkdir -p $(@D)
	ln -sf $(SOFTFLOAT_BUILD_PATH)/softfloat.a $@

clean-softfloat:
	$(MAKE) -s -C $(SOFTFLOAT_BUILD_PATH) clean

.PHONY: $(SOFTFLOAT) clean-softfloat
