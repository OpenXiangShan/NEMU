CPT_CROSS_COMPILE ?= riscv64-linux-gnu-

CPT_REPO = $(RESOURCE_PATH)/gcpt_restore
CPT_BIN = $(CPT_REPO)/build/gcpt.bin
CPT_MEMLAYOUT_HEADER = $(CPT_REPO)/src/restore_rom_addr.h
GCPT_RESTORER_SRC = $(CPT_REPO)/build/generated/gcpt_restorer.c

ifeq ($(wildcard $(CPT_MEMLAYOUT_HEADER)),)
  $(shell git clone --depth=1 https://github.com/OpenXiangShan/LibCheckpointAlpha.git $(CPT_REPO) 1>&2)
endif

ifdef CONFIG_HAS_FLASH
SRCS-y += $(GCPT_RESTORER_SRC)
endif

gcpt_restore_bin: $(CPT_BIN)

$(CPT_BIN):
	$(Q)$(MAKE) $(silent) -C $(CPT_REPO) CROSS_COMPILE=$(CPT_CROSS_COMPILE)

$(GCPT_RESTORER_SRC): $(CPT_BIN) $(NEMU_HOME)/scripts/gen_gcpt_restorer_c.py
	@mkdir -p $(@D)
	$(Q)python3 $(NEMU_HOME)/scripts/gen_gcpt_restorer_c.py $< $@

clean-libcheckpointalpha:
	$(Q)$(MAKE) -s -C $(CPT_REPO) clean

.PHONY: gcpt_restore_bin clean-libcheckpointalpha
