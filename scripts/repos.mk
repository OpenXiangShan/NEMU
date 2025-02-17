RESOURCE_PATH := resource

include $(NEMU_HOME)/scripts/softfloat.mk
include $(NEMU_HOME)/scripts/LibcheckpointAlpha.mk
include $(NEMU_HOME)/scripts/nanopb.mk

clean-repos: clean-softfloat clean-libcheckpointalpha
