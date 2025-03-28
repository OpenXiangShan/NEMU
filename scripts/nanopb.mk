NANOPB_CROSS_COMPILE ?= riscv64-unknown-linux-gnu-

NANOPB_REPO = $(RESOURCE_PATH)/nanopb

ifeq ($(wildcard $(NANOPB_REPO)),)
  $(shell git clone --depth=1 https://github.com/nanopb/nanopb.git $(NANOPB_REPO) 1>&2)
endif

