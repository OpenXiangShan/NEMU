GUEST_ISA ?= x86
ISAS = $(shell ls $(NEMU_HOME)/src/isa/)
ifeq ($(filter $(ISAS), $(GUEST_ISA)), ) # GUEST_ISA must be valid
$(error Invalid GUEST_ISA=$(GUEST_ISA). Supported: $(ISAS))
endif
NAME := $(GUEST_ISA)-$(NAME)
CFLAGS += -D__ISA_$(GUEST_ISA)__=1
