SRCS-y += src/nemu-main.c
DIRS-y = src/cpu src/monitor src/utils
DIRS-$(CONFIG_MODE_SYSTEM) += src/memory

LIBS += $(if $(CONFIG_SHARE),,-lreadline -ldl -pie)
ifdef CONFIG_SHARE
SHARE = 1
endif
