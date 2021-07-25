ifdef CONFIG_FPU_SOFT
SOFTFLOAT = resource/softfloat/build/softfloat.a
SPECIALIZE_TYPE = $(if $(CONFIG_ISA_riscv64),RISCV,8086-SSE)
INC_PATH += resource/softfloat/repo/source/include
INC_PATH += resource/softfloat/repo/source/$(SPECIALIZE_TYPE)
ARCHIVES += $(SOFTFLOAT)
$(SOFTFLOAT):
	SPECIALIZE_TYPE=$(SPECIALIZE_TYPE) $(MAKE) -s -C resource/softfloat/

.PHONY: $(SOFTFLOAT)
else ifdef CONFIG_FPU_HOST
LIBS += -lm
endif
