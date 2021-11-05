ifneq ($(CONFIG_ITRACE)$(CONFIG_IQUEUE),)
CXXSRC = src/utils/disasm.cc
CXXFLAGS += $(shell llvm-config-11 --cxxflags) -fPIE
LIBS += $(shell llvm-config-11 --libs)
endif

ifndef CONFIG_IQUEUE
	DIRS-BLACKLIST-y += src/utils/iqueue.c
endif
