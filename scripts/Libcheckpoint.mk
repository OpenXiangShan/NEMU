LIBCHECKPOINT_CROSS_COMPILE ?= riscv64-unknown-linux-gnu-

LIBCHECKPOINT_REPO = $(RESOURCE_PATH)/LibCheckpoint
LIBCHECKPOINT_BIN = $(LIBCHECKPOINT_REPO)/build/gcpt.bin
LIBCHECKPOINT_MEMLAYOUT_HEADER = $(LIBCHECKPOINT_REPO)/src/checkpoint.proto

ifeq ($(wildcard $(LIBCHECKPOINT_MEMLAYOUT_HEADER)),)
  $(shell git clone --depth=1 https://github.com/OpenXiangShan/LibCheckpoint.git $(LIBCHECKPOINT_REPO))
  $(shell cd $(LIBCHECKPOINT_REPO) && git submodule update --init && cd - )
endif

libcheckpoint_bin: $(LIBCHECKPOINT_BIN)

$(LIBCHECKPOINT_BIN):
	$(Q)$(MAKE) $(silent) -C $(LIBCHECKPOINT_REPO) CROSS_COMPILE=$(LIBCHECKPOINT_CROSS_COMPILE)

clean-libcheckpoint:
	$(Q)$(MAKE) -s -C $(LIBCHECKPOINT_REPO) clean

.PHONY: libcheckpoint_bin clean-libcheckpoint

