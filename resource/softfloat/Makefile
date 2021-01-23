REPO_PATH = repo
ifeq ($(wildcard repo/COPYING.txt),)
  $(shell git clone --depth=1 https://github.com/ucb-bar/berkeley-softfloat-3 $(REPO_PATH))
endif

BUILD_PATH = $(abspath $(REPO_PATH)/build/Linux-x86_64-GCC)

EXTRA_SRCS = $(shell find extra -name "*.c")
EXTRA_OBJS = $(EXTRA_SRCS:%.c=build/%.o)
CFLAGS = -O2 -MMD -Wall -Werror -I./repo/source/include -I./repo/source/RISCV

-include $(EXTRA_OBJS:.o=.d)

build/%.o: %.c
	@echo + CC $<
	@mkdir -p $(dir $@)
	@gcc $(CFLAGS) -c -o $@ $<

all: $(EXTRA_OBJS)
	$(MAKE) -C $(BUILD_PATH) all
	mkdir -p build/
	cp $(BUILD_PATH)/softfloat.a build/softfloat.a
	ar rcs build/softfloat.a $(EXTRA_OBJS)

.PHONY: all
