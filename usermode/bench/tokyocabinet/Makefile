.PHONY: all-experiments
all-experiments: library/libtokyocabinet.a experiment/mmap experiment/mnemosyne

experiment/mmap: experiment/mmap.c library-reg/libtokyocabinet.a
	@echo "(COMPILE) $@"
	@icc -Qtm_enabled -o $@ $< -Ilibrary-reg -I../../library/mtm/include -lpthread library-reg/libtokyocabinet.a -lbz2 -lz

experiment/mnemosyne: experiment/mnemosyne.c library/libtokyocabinet.a
	@echo "(COMPILE) $@"
	@icc -fpic -fno-omit-frame-pointer -m64 -T ../../util/linker_script_persistent_segment_m64 -g -O2 -Qtm_enabled -o $@ -I ../../library -I../../library/mnemosyne/include -I../../library/mtm/include -Ilibrary -lpthread library/libtokyocabinet.a -L../../build/library/mnemosyne -lmnemosyne -L../../build/library/malloc -lmalloc -lbz2 -lz $^

library/libtokyocabinet.a: library/Makefile library/tcbdb.h library/tcbdb.c
	@echo "(BUILD) Tokyo Cabinet for Mnemosyne"
	@make -C library

library-reg/libtokyocabinet.a: library-reg/Makefile library/tcbdb.h library/tcbdb.c
	@echo "(BUILD) Tokyo Cabinet"
	@make -C library-reg

library/Makefile: library/Makefile.in
	@echo "(CONFIGURE) Tokyo Cabinet"
	@cd library; ./configure --disable-shared
