LEV_DIR		= /home/hvolos/workspace@multifacet/mnemosyne/mnemosyne/usermode/bench/paxos/libevent-1.4.12-stable
CFLAGS 		= -Qtm_enabled -g -O2 -Wall -Wshadow -Wextra -fno-omit-frame-pointer
CC			= icc

# Do not modify the following
OBJS		= $(subst .c,.o,$(SRCS))
CPPFLAGS	= -I../include/ -I../ -I$(LEV_DIR)
DEPS 		= $(patsubst %.c,.deps/%.d,$(SRCS))
SNAME		:= $(shell uname)

all: $(OBJS) $(LIBS) $(PROGRAMS) aux

-include $(DEPS)
	
%: %.o
	@echo "> $@ <"
	$(QUIET) $(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	@echo "> $@ <"
	$(QUIET) test -d .deps || mkdir .deps;
	$(QUIET) $(CC) $(CPPFLAGS) $(CFLAGS) $< -MT $@ -MM -MP -MF .deps/$*.d
	$(QUIET) $(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

clean:
	$(QUIET) rm -f $(PROGRAMS) $(LIBS) $(OBJS) $(AUX_FILES)
	$(QUIET) rm -drf  .deps
	
.PHONY: clean $(AUX_ALL) aux
