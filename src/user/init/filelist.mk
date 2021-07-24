SRCS-BLACKLIST += $(shell find src/user/init/ -name "*.c" | grep -v "$(ISA).c$$")
