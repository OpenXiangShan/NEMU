NAME = x86-syscall
SRCS = syscall.S

BUILD_DIR = ./build
ELF = $(BUILD_DIR)/$(NAME).elf

$(ELF): $(SRCS)
	mkdir -p $(dir $(ELF))
	i686-linux-gnu-gcc -static -c -o $@.o $^
	i686-linux-gnu-ld -o $@ $@.o
