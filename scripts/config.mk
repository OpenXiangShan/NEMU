Q            := @
KCONFIG_PATH := $(NEMU_HOME)/tools/kconfig
Kconfig      := $(NEMU_HOME)/Kconfig
rm-distclean += include/generated include/config .config
silent := -s

CONF  := $(KCONFIG_PATH)/build/conf
MCONF := $(KCONFIG_PATH)/build/mconf

$(CONF):
	$(Q)$(MAKE) $(silent) -C $(KCONFIG_PATH) NAME=conf

$(MCONF):
	$(Q)$(MAKE) $(silent) -C $(KCONFIG_PATH) NAME=mconf

menuconfig: $(MCONF) $(MCONF)
	$(Q)$(MCONF) $(Kconfig)
	$(Q)$(CONF) $(silent) --syncconfig $(Kconfig)

savedefconfig: $(CONF)
	$(Q)$< $(silent) --$@=configs/defconfig $(Kconfig)

%defconfig: $(CONF)
	$(Q)$< $(silent) --defconfig=configs/$@ $(Kconfig)

# Help text used by make help
help:
	@echo  '  menuconfig	  - Update current config utilising a menu based program'
	@echo  '  savedefconfig   - Save current config as ./defconfig (minimal config)'

distclean:
	rm -rf $(rm-distclean)

PHONY += savedefconfig defconfig
.PHONY: $(PHONY)
