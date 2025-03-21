LIBCHECKPOINT_CROSS_COMPILE ?= riscv64-unknown-linux-gnu-

LIBCHECKPOINT_REPO = $(RESOURCE_PATH)/LibCheckpoint
LIBCHECKPOINT_BIN = $(LIBCHECKPOINT_REPO)/build/gcpt.bin
LIBCHECKPOINT_MEMLAYOUT_HEADER = $(LIBCHECKPOINT_REPO)/src/checkpoint.proto

ifeq ($(wildcard $(LIBCHECKPOINT_MEMLAYOUT_HEADER)),)
  $(shell git clone --depth=1 https://github.com/OpenXiangShan/LibCheckpoint.git $(LIBCHECKPOINT_REPO))
  $(shell cd $(LIBCHECKPOINT_REPO) && git submodule update --init && cd - )
endif

LIBCHECKPOINT_INC_FILE = $(LIBCHECKPOINT_REPO)/include/checkpoint.pb.h $(LIBCHECKPOINT_REPO)/resource/nanopb/pb.h
CFLAGS += -I$(NANOPB_REPO)
CFLAGS += -I$(LIBCHECKPOINT_REPO)/export_include/
SRCS-y += $(LIBCHECKPOINT_REPO)/src/checkpoint.pb.c
SRCS-y += $(NANOPB_REPO)/pb_common.c
SRCS-y += $(NANOPB_REPO)/pb_decode.c
SRCS-y += $(NANOPB_REPO)/pb_encode.c

$(LIBCHECKPOINT_REPO)/src/checkpoint.pb.c: $(LIBCHECKPOINT_BIN)

libcheckpoint_bin: $(LIBCHECKPOINT_BIN)

$(LIBCHECKPOINT_BIN):
	$(Q)$(MAKE) $(silent) -C $(LIBCHECKPOINT_REPO) CROSS_COMPILE=$(LIBCHECKPOINT_CROSS_COMPILE)

clean-libcheckpoint:
	$(Q)$(MAKE) -s -C $(LIBCHECKPOINT_REPO) clean

.PHONY: libcheckpoint_bin clean-libcheckpoint

