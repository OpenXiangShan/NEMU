#***************************************************************************************
# Copyright (c) 2014-2021 Zihao Yu, Nanjing University
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

NAME  = kvm
SRCS  = $(shell find src/ -name "*.c")

SHARE = 1

include $(NEMU_HOME)/include/config/auto.conf
include $(NEMU_HOME)/include/config/auto.conf.cmd
include $(NEMU_HOME)/scripts/utils.mk
include $(NEMU_HOME)/scripts/isa.mk

INC_DIR += $(NEMU_HOME)/include $(NEMU_HOME)/src/isa/$(ISA)/include

include $(NEMU_HOME)/scripts/build.mk
