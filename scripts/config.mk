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

COLOR_RED := $(shell echo "\033[1;31m")
COLOR_END := $(shell echo "\033[0m")

ifeq ($(wildcard .config),)
$(warning $(COLOR_RED)Warning: .config does not exists!$(COLOR_END))
$(warning $(COLOR_RED)To build the project, first run 'make menuconfig'.$(COLOR_END))
endif

Q            := @
KCONFIG_PATH := $(NEMU_HOME)/tools/kconfig
FIXDEP_PATH  := $(NEMU_HOME)/tools/fixdep
Kconfig      := $(NEMU_HOME)/Kconfig
CONFIG_MK     := $(NEMU_HOME)/scripts/merge_config.sh
rm-distclean += include/generated include/config .config .config.old \
                .config.fragments.*
silent := -s

CONF   := $(KCONFIG_PATH)/build/conf
MCONF  := $(KCONFIG_PATH)/build/mconf
FIXDEP := $(FIXDEP_PATH)/build/fixdep

$(CONF):
	$(Q)$(MAKE) $(silent) -C $(KCONFIG_PATH) NAME=conf

$(MCONF):
	$(Q)$(MAKE) $(silent) -C $(KCONFIG_PATH) NAME=mconf

$(FIXDEP):
	$(Q)$(MAKE) $(silent) -C $(FIXDEP_PATH)

menuconfig: $(MCONF) $(CONF) $(FIXDEP)
	$(Q)$(MCONF) $(Kconfig)
	$(Q)$(CONF) $(silent) --syncconfig $(Kconfig)

savedefconfig: $(CONF)
	$(Q)$< $(silent) --$@=configs/defconfig $(Kconfig)

# A target (configs/xxx_defconfig) is a composition manifest: "# layer: <path>"
# lines name a machine profile under configs/<isa>/ plus optional fragments
# under configs/fragments/.  Merge the layers first, then let Kconfig install
# the result exactly like a full .config.
%defconfig: $(CONF) $(FIXDEP) $(CONFIG_MK)
	$(Q)layers=$$(sed -n 's|^# layer: |configs/|p' configs/$@); \
		merged=$$(mktemp $(NEMU_HOME)/.config.fragments.XXXXXX) && \
		$(CONFIG_MK) -o $$merged $$layers configs/$@ && \
		$(CONF) $(silent) --defconfig=$$merged $(Kconfig) && \
		$(CONF) $(silent) --syncconfig $(Kconfig); \
		status=$$?; rm -f $$merged; exit $$status

# Check that every configuration target composes (used by CI).
config-check: $(CONF)
	$(Q)$(NEMU_HOME)/scripts/check_configs.sh

.PHONY: menuconfig savedefconfig defconfig config-check

# Help text used by make help
help:
	@echo  '  menuconfig	  - Update current config utilising a menu based program'
	@echo  '  savedefconfig   - Save current config as configs/defconfig (minimal config)'
	@echo  '  config-check    - Verify that every configs/*_defconfig target composes'

distclean: clean
	-@rm -rf $(rm-distclean)

.PHONY: help distclean

define call_fixdep
	@$(FIXDEP) $(1) $(2) unused > $(1).tmp
	@mv $(1).tmp $(1)
endef
