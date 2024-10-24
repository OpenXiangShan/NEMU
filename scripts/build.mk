.DEFAULT_GOAL = app

ifdef SHARE
SO = -so
CFLAGS  += -fPIC -D_SHARE=1
LDFLAGS += -rdynamic -shared -fPIC -Wl,--no-undefined -lz
endif

WORK_DIR  = $(shell pwd)
BUILD_DIR = $(WORK_DIR)/build

INC_DIR += $(WORK_DIR)/include $(NEMU_HOME)/lib-include
XINC_DIR = $(INC_DIR) $(WORK_DIR)/resource
OBJ_DIR  = $(BUILD_DIR)/obj-$(NAME)$(SO)
BINARY   = $(BUILD_DIR)/$(NAME)$(SO)

CC ?= gcc
CXX = g++

ifdef PGO_PROF
PGO_FLAGS = -fprofile-generate -fprofile-dir=$(NEMU_HOME)/profile
else ifdef PGO_USE
PGO_FLAGS = -fprofile-use -fprofile-dir=$(NEMU_HOME)/profile
else
PGO_FLAGS =
endif

CCACHE := $(if $(shell which ccache),ccache,)

# Compilation flags
CC := $(CCACHE) $(CC)
LD := $(CCACHE) $(CXX)
INCLUDES = $(addprefix -I, $(INC_DIR))
XINCLUDES = $(addprefix -I, $(XINC_DIR))
CFLAGS  := -O2 -ftree-vectorize -MMD -Wall -Werror -fno-strict-aliasing $(INCLUDES) $(CFLAGS) $(PGO_FLAGS)
CXXFLAGS  := --std=c++17 $(XINCLUDES) $(CFLAGS)
LDFLAGS := -O2 $(LDFLAGS) $(PGO_FLAGS)
# filesystem
ifndef SHARE
LDFLAGS += -lstdc++fs -lstdc++ -lm
endif

COBJS = $(SRCS:%.c=$(OBJ_DIR)/%.o)
XOBJS = $(XSRCS:%.cpp=$(OBJ_DIR)/%.opp)

ifndef SHARE
OBJS = $(COBJS) $(XOBJS)
else
OBJS = $(COBJS) $(XOBJS)
endif

ifdef CONFIG_MEM_COMPRESS
LDFLAGS += -lzstd
endif

# Compilation patterns
$(OBJ_DIR)/%.o: %.c
	@echo + CC $<
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) $(SO_CFLAGS) -c -o $@ $<
	@$(CC) $(CFLAGS) -E $(SO_CFLAGS) -c -o $@.c $<
	$(call call_fixdep, $(@:.o=.d), $@)

$(OBJ_DIR)/%.opp: %.cpp
	@echo + CXX $<
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) -c -o $@ $<
	$(call call_fixdep, $(@:.opp=.d), $@)

# Dependencies
-include $(COBJS:.o=.d) $(XOBJS:.opp=.d)

# Some convenient rules

.PHONY: app clean

app: $(BINARY)

$(BINARY): $(OBJS) $(LIBS)
	@echo + $(LD) $@
	@$(LD) -o $@ $(OBJS) $(LDFLAGS) $(LIBS)

staticlib: $(BUILD_DIR)/lib$(NAME).a

$(BUILD_DIR)/lib$(NAME).a: $(OBJS) $(LIBS)
	@echo + AR $@
	@ar rcs $(BUILD_DIR)/lib$(NAME).a $(OBJS) $(LIBS)

.PHONY: clean-softfloat
clean: clean-softfloat
	-rm -rf $(BUILD_DIR)
