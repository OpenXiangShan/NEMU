RESOURCE_PATH := resource

include $(NEMU_HOME)/scripts/softfloat.mk
include $(NEMU_HOME)/scripts/Libcheckpoint.mk
include $(NEMU_HOME)/scripts/LibcheckpointAlpha.mk

clean-repos: clean-softfloat clean-libcheckpointalpha clean-libcheckpoint
