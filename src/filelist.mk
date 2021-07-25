SRCS-y += src/nemu-main.c
DIRS-y = src/cpu src/monitor src/utils
DIRS-$(CONFIG_MODE_SYSTEM) += src/memory

ifdef CONFIG_SHARE
SHARE = 1
else
LIBS += $(if $(CONFIG_AM),,-lreadline -ldl -pie)
endif
