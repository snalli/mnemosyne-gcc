.PHONY: all-experiments
all-experiments: library/libtokyocabinet.a experiment/mmap experiment/mmap-nosync experiment/mnemosyne
	@echo "(RUN) All Experiments"
	@perl experiment/runner.pl

experiment/mmap: experiment/mmap.c
	@echo "(COMPILE) $@"
	@icc -Qtm_enabled -o $@ $< -Ilibrary -I../../library/mtm/include -lpthread library/libtokyocabinet.a -lbz2 -lz

experiment/mmap-nosync: experiment/mmap-nosync.c
	@echo "(COMPILE) $@"
	@icc -Qtm_enabled -o $@ $< -Ilibrary -I../../library/mtm/include -lpthread library/libtokyocabinet.a -lbz2 -lz

experiment/mnemosyne: experiment/mnemosyne.c
	@echo "(COMPILE) $@"
	@icc -g -O0 -Qtm_enabled -o $@ $< -I../../library/mtm/include -Ilibrary -lpthread library/libtokyocabinet.a ../../build/library/mnemosyne/libmnemosyne.so ../../build/library/mtm/libmtm.so -lbz2 -lz

library/libtokyocabinet.a: library/Makefile library/tcbdb.h library/tcbdb.c
	@echo "(BUILD) Tokyo Cabinet"
	@make -C library

library/Makefile: library/Makefile.in
	@echo "(CONFIGURE) Tokyo Cabinet"
	@cd library; ./configure --disable-shared
