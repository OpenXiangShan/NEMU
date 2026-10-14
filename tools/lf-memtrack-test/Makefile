# Standalone unit test of the leader-follower page write tracker.
NEMU_HOME ?= $(abspath ../..)
CFLAGS += -O2 -Wall -Werror -DLF_MEMTRACK_STANDALONE -I$(NEMU_HOME)/include

memtrack_test: memtrack_test.c $(NEMU_HOME)/src/profiling/lf_memtrack.c $(NEMU_HOME)/include/profiling/lf_memtrack.h
	$(CC) $(CFLAGS) -o $@ memtrack_test.c $(NEMU_HOME)/src/profiling/lf_memtrack.c

clean:
	rm -f memtrack_test

.PHONY: clean
