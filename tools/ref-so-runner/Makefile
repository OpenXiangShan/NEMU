#***************************************************************************************
# Copyright (c) 2026 Institute of Computing Technology, Chinese Academy of Sciences
#
# NEMU is licensed under Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#          http://license.coscl.org.cn/MulanPSL2
#
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
# EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
# MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
#
# See the Mulan PSL v2 for more details.
#**************************************************************************************/

CC ?= gcc
CFLAGS ?= -O2 -g
LDFLAGS ?=

TARGET := runner

.PHONY: all clean

all: $(TARGET)

$(TARGET): runner.c
	$(CC) $(CFLAGS) -Wall -Wextra -Werror -std=c11 -o $@ $< -ldl $(LDFLAGS)

clean:
	rm -f $(TARGET) ref-so-runner
