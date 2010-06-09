.PHONY: all-experiments
ICCPATH=/opt/intel/Compiler/11.0/606/
MTM_LIB=-L/opt/intel/Compiler/11.0/606/lib/intel64 -litmdyn
#MTM_LIB=-L../../build/library/mtm/ -lmtm

all-experiments: library/libtokyocabinet.a experiment/mmap experiment/mnemosyne

experiment/mmap: experiment/mmap.c library-reg/libtokyocabinet.a
	@echo "(COMPILE) $@"
	@icc -Qtm_enabled -o $@ $< -Ilibrary-reg -I../../library/mtm/include -lpthread library-reg/libtokyocabinet.a -lbz2 -lz

experiment/mnemosyne: experiment/mnemosyne.o library/libtokyocabinet.a
	@echo "(LINK) $@"
	icc -fpic -fno-omit-frame-pointer -m64 -T ../../util/linker_script_persistent_segment_m64 -g -O2 -o $@ -I ../../library -I../../library/mnemosyne/include -I../../library/mtm/include -Ilibrary -lpthread library/libtokyocabinet.a -L../../build/library/mnemosyne -lmnemosyne -L../../build/library/malloc -lmalloc $(MTM_LIB) $(ICCPATH)/lib/intel64/libitm.a -lbz2 -lz $^

experiment/mnemosyne.o: experiment/mnemosyne.c
	@echo "(COMPILE) $@"
	@icc -fpic -fno-omit-frame-pointer -m64  -g -O2 -Qtm_enabled -c -o $@ -I ../../library -I../../library/mnemosyne/include -I../../library/mtm/include -Ilibrary $^

library/libtokyocabinet.a: library/Makefile library/tcbdb.h library/tcbdb.c
	@echo "(BUILD) Tokyo Cabinet for Mnemosyne"
	@make -C library

library-reg/libtokyocabinet.a: library-reg/Makefile library/tcbdb.h library/tcbdb.c
	@echo "(BUILD) Tokyo Cabinet"
	@make -C library-reg

library/Makefile: library/Makefile.in
	@echo "(CONFIGURE) Tokyo Cabinet"
	@cd library; ./configure --disable-shared
