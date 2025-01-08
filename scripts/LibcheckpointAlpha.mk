LIB_CPT_PATH = resource/gcpt_restore
CPT_BIN = $(LIB_CPT_PATH)/build/gcpt.bin
CPT_LAYOUT_HEADER = $(LIB_CPT_PATH)/src/restore_rom_addr.h

CPT_CROSS_COMPILE ?= riscv64-unknown-linux-gnu-

$(CPT_LAYOUT_HEADER):
	$(shell git clone https://github.com/OpenXiangShan/LibCheckpointAlpha.git $(LIB_CPT_PATH))

$(CPT_BIN): $(CPT_LAYOUT_HEADER)
	$(Q)$(MAKE) $(silent) -C $(LIB_CPT_PATH) CROSS_COMPILE=$(CPT_CROSS_COMPILE)

clean-libcheckpointalpha: $(CPT_LAYOUT_HEADER)
	$(Q)$(MAKE) -s -C $(LIB_CPT_PATH) clean

.PHONY: clean-libcheckpointalpha
