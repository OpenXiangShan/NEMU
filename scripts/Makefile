.DEFAULT_GOAL = app

ISA ?= x86
ISAS = $(shell ls $(NEMU_HOME)/src/isa/)
ifeq ($(filter $(ISAS), $(ISA)), ) # ISA must be valid
$(error Invalid ISA=$(ISA). Supported: $(ISAS))
endif

ifneq ($(MAKECMDGOALS),clean)
$(info Building $(ISA)-$(NAME))
endif

ifdef SHARE
SO = -so
CFLAGS  += -fPIC -D_SHARE=1
LDFLAGS += -rdynamic -shared -fPIC
endif

WORK_DIR  = $(shell pwd)
BUILD_DIR = $(WORK_DIR)/build

INC_DIR += $(WORK_DIR)/include $(NEMU_HOME)/lib-include
OBJ_DIR  = $(BUILD_DIR)/obj-$(ISA)-$(NAME)$(SO)
BINARY   = $(BUILD_DIR)/$(ISA)-$(NAME)$(SO)


CCACHE := $(if $(shell which ccache),ccache,)

GCC ?= gcc

# Compilation flags
CC = $(CCACHE) $(GCC)
LD = $(CCACHE) $(GCC)
INCLUDES = $(addprefix -I, $(INC_DIR))
CFLAGS  := -O2 -MMD -Wall -Werror $(INCLUDES) -D__ISA_$(ISA)__ $(CFLAGS)
LDFLAGS := -O2 $(LDFLAGS)

OBJS = $(SRCS:src/%.c=$(OBJ_DIR)/%.o)

# Compilation patterns
$(OBJ_DIR)/%.o: src/%.c
	@echo + CC $<
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) $(SO_CFLAGS) -c -o $@ $<

# Depencies
-include $(OBJS:.o=.d)

# Some convenient rules

.PHONY: app clean

app: $(BINARY)

$(BINARY): $(OBJS) $(LIBS)
	@echo + LD $@
	@$(LD) -o $@ $(OBJS) $(LDFLAGS) $(LIBS)

clean:
	-rm -rf $(BUILD_DIR)
