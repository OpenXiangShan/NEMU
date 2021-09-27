ifdef CONFIG_TARGET_NATIVE_ELF
CXXSRC = src/utils/disasm.cc
CXXFLAGS += $(shell llvm-config-11 --cxxflags) -fPIE
LIBS += $(shell llvm-config-11 --libs)
endif
