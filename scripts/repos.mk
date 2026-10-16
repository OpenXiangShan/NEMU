RESOURCE_PATH := $(NEMU_HOME)/resource

include $(NEMU_HOME)/scripts/softfloat.mk
include $(NEMU_HOME)/scripts/nanopb.mk
include $(NEMU_HOME)/scripts/Libcheckpoint.mk
include $(NEMU_HOME)/scripts/LibcheckpointAlpha.mk

ifneq ($(filter y,$(CONFIG_FAST_REF) $(CONFIG_STORE_LOG_HASH)),)
SRCS-y += resource/xxhash/xxhash.c
INC_DIR += $(RESOURCE_PATH)/xxhash
CFLAGS += -DXXH_NAMESPACE=nemu_xxh_
endif

clean-repos: clean-softfloat clean-libcheckpointalpha clean-libcheckpoint
