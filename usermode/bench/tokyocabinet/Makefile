.PHONY: all-experiments
all-experiments: library/libtokyocabinet.a experiment/mmap experiment/mmap-nosync
	@echo "(RUN) All Experiments"
	@perl experiment/runner.pl

experiment/mmap: experiment/mmap.c
	@echo "(COMPILE) $@"
	@$(CXX) -o $@ $< library/libtokyocabinet.a -lbz2 -lz

experiment/mmap-nosync: experiment/mmap-nosync.c
	@echo "(COMPILE) $@"
	@$(CXX) -o $@ $< library/libtokyocabinet.a -lbz2 -lz

library/libtokyocabinet.a: library/Makefile
	@echo "(BUILD) Tokyo Cabinet"
	@make -C library

library/Makefile:
	@echo "(CONFIGURE) Tokyo Cabinet"
	@cd library; ./configure
