CPT_CROSS_COMPILE ?= riscv64-unknown-linux-gnu-

CPT_REPO = $(RESOURCE_PATH)/gcpt_restore
CPT_BIN = $(CPT_REPO)/build/gcpt.bin
CPT_MEMLAYOUT_HEADER = $(CPT_REPO)/src/restore_rom_addr.h

ifeq ($(wildcard $(CPT_MEMLAYOUT_HEADER)),)
  $(shell git clone --depth=1 https://github.com/OpenXiangShan/LibCheckpointAlpha.git $(CPT_REPO))
endif

$(CPT_BIN):
ifdef CONFIG_HAS_FLASH
	$(Q)$(MAKE) $(silent) -C $(CPT_REPO) CROSS_COMPILE=$(CPT_CROSS_COMPILE)
endif

clean-libcheckpointalpha:
	$(Q)$(MAKE) -s -C $(CPT_REPO) clean

.PHONY: clean-libcheckpointalpha
