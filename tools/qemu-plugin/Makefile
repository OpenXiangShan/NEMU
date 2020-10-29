INC_DIR += ./include
BUILD_DIR ?= ./build
OBJ_DIR ?= $(BUILD_DIR)/obj
BINARY ?= $(BUILD_DIR)/qemu-so

.DEFAULT_GOAL = app

# Compilation flags
CC = gcc
LD = gcc
INCLUDES  = $(addprefix -I, $(INC_DIR))
CFLAGS   += -O2 -fPIC -MMD -Wall -Werror -DNEMU_HOME=$(NEMU_HOME) $(INCLUDES)

# Files to be compiled
SRCS = $(shell find src/ -name "*.c")
OBJS = $(SRCS:src/%.c=$(OBJ_DIR)/%.o)

# Compilation patterns
$(OBJ_DIR)/%.o: src/%.c
	@echo + CC $<
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS)  -c -o $@ $<


# Depencies
-include $(OBJS:.o=.d)

# Some convinient rules

.PHONY: app clean
app: $(BINARY)

$(BINARY): $(OBJS)
	@echo + LD $@
	@$(LD) -O2 -rdynamic -shared -fPIC -o $@ $^

QEMU = /home/yzh/software/qemu-v3.1.0/i386-softmmu/qemu-system-i386
run: app
	LD_PRELOAD=$(BINARY) $(QEMU)

gdb: app
	gdb --args env LD_PRELOAD=$(BINARY) $(QEMU)

clean: 
	-rm -rf $(BUILD_DIR)
