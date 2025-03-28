RESOURCE_PATH := $(NEMU_HOME)/resource

include $(NEMU_HOME)/scripts/softfloat.mk
include $(NEMU_HOME)/scripts/nanopb.mk
include $(NEMU_HOME)/scripts/Libcheckpoint.mk
include $(NEMU_HOME)/scripts/LibcheckpointAlpha.mk

clean-repos: clean-softfloat clean-libcheckpointalpha clean-libcheckpoint
