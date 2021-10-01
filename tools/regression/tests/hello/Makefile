ISA ?= x86
NAME = hello-$(ISA)
SRCS = hello.c

ifeq ($(ISA),x86)
CROSS_COMPILE = i686-linux-gnu-
else ifeq ($(ISA),riscv64)
CROSS_COMPILE = riscv64-linux-gnu-
else
$(error Unsupported ISA=$(ISAS))
endif

BUILD_DIR = ./build
ELF = $(BUILD_DIR)/$(NAME).elf

$(ELF): $(SRCS)
	mkdir -p $(dir $(ELF))
	$(CROSS_COMPILE)gcc -fno-pie -static -o $@ $^
