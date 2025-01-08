include $(NEMU_HOME)/scripts/softfloat.mk
include $(NEMU_HOME)/scripts/LibcheckpointAlpha.mk

clean-repos: clean-softfloat clean-libcheckpointalpha
