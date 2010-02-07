LIBPAXOS	= libpaxos.a
MODULES 	= lib tests
AR			= ar
ARFLAGS		= rc
#QUIET		= @

all: libevent_check $(LIBPAXOS) $(MODULES)
	@echo "Build completed"

.PHONY: $(MODULES) libevent_check

$(MODULES):	
	$(QUIET) $(MAKE) QUIET=$(QUIET) --directory=$@ || exit 1;
	@echo "Done in $@/"
	
$(LIBPAXOS): lib 
	$(QUIET) $(AR) $(ARFLAGS) $@ $(addsuffix /*.o, $^)
	$(QUIET) ranlib $@
	@echo "Libpaxos.a done."

libevent_check:
	$(QUIET) if [ ! -e $(LEV_DIR) ]; then \
	echo "Error: LibEvent not found in $(LEV_DIR)"; \
	echo "You must build LibEvent and update the LEV_DIR variable in Makefile.inc before proceeding"; \
	exit 1; \
	fi

clean:
	$(QUIET) rm -f $(LIBPAXOS)
	$(QUIET) for m in $(MODULES); do \
		$(MAKE) QUIET=$(QUIET) clean --directory=$$m; \
	done
