STUID = 191220000
STUNAME = 张三

# DO NOT modify the following code!!!

GITFLAGS = -q --author='tracer-ics2020 <tracer@njuics.org>' --no-verify --allow-empty

#ifndef __ICS_EXPORT
ifdef __NOT_DEFINED
#else
# prototype: git_commit(msg)
define git_commit
	-@git add $(NEMU_HOME)/.. -A --ignore-errors
	-@while (test -e .git/index.lock); do sleep 0.1; done
	-@(echo "> $(1)" && echo $(STUID) && hostnamectl && uptime) | git commit -F - $(GITFLAGS)
	-@sync
endef
#endif
#ifndef __ICS_EXPORT
endif
#endif
