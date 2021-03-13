ISA ?= x86
ISAS = $(shell ls $(NEMU_HOME)/src/isa/)
ifeq ($(filter $(ISAS), $(ISA)), ) # ISA must be valid
$(error Invalid ISA=$(ISA). Supported: $(ISAS))
endif
NAME := $(ISA)-$(NAME)
CFLAGS += -D__ISA_$(ISA)__=1
