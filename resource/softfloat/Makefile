REPO_PATH = repo
ifeq ($(wildcard repo/COPYING.txt),)
  $(shell git clone --depth=1 https://github.com/ucb-bar/berkeley-softfloat-3 $(REPO_PATH))
endif

BUILD_PATH = $(abspath $(REPO_PATH)/build/Linux-x86_64-GCC)

all:
	$(MAKE) -C $(BUILD_PATH) all
	mkdir -p build/
	ln -sf $(BUILD_PATH)/softfloat.a build/softfloat.a

clean:
	$(MAKE) -C $(BUILD_PATH) clean

.PHONY: all
