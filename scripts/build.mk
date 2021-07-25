.DEFAULT_GOAL = app

ifdef SHARE
SO = -so
CFLAGS  += -fPIC -D_SHARE=1
LDFLAGS += -rdynamic -shared -fPIC -Wl,--no-undefined
endif

WORK_DIR  = $(shell pwd)
BUILD_DIR = $(WORK_DIR)/build

INC_DIR += $(WORK_DIR)/include $(NEMU_HOME)/lib-include
OBJ_DIR  = $(BUILD_DIR)/obj-$(NAME)$(SO)
BINARY   = $(BUILD_DIR)/$(NAME)$(SO)

CC ?= gcc

CCACHE := $(if $(shell which ccache),ccache,)

# Compilation flags
CC := $(CCACHE) $(CC)
LD := $(CCACHE) $(CC)
INCLUDES = $(addprefix -I, $(INC_DIR))
CFLAGS  := -O2 -MMD -Wall -Werror $(INCLUDES) $(CFLAGS)
LDFLAGS := -O2 $(LDFLAGS)

OBJS = $(SRCS:%.c=$(OBJ_DIR)/%.o)

# Compilation patterns
$(OBJ_DIR)/%.o: %.c
	@echo + CC $<
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) $(SO_CFLAGS) -c -o $@ $<
	$(call call_fixdep, $(@:.o=.d), $@)

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
