# Makefile for sample programs of Tokyo Cabinet



#================================================================
# Setting Variables
#================================================================


# Generic settings
SHELL = /bin/sh

# Targets
MYBINS = tcutilex tchdbex tcbdbex tcfdbex tctdbex tcadbex tctchat.cgi tctsearch.cgi

# Building binaries
CC = gcc
CFLAGS = -I. -I.. -ansi -Wall -pedantic -fsigned-char -O2
C99FLAGS = -I. -I.. -std=c99 -Wall -pedantic -fsigned-char -O2
LDFLAGS =
LIBS = -L. -L.. -ltokyocabinet -lz -lbz2 -lpthread -lm -lc
LDENV = LD_RUN_PATH=/lib:/usr/lib:$(HOME)/lib:/usr/local/lib:.:..



#================================================================
# Suffix rules
#================================================================


.SUFFIXES :
.SUFFIXES : .c .o

.c.o :
	$(CC) -c $(CFLAGS) $<



#================================================================
# Actions
#================================================================


all : $(MYBINS)


clean :
	rm -rf $(MYBINS) *.exe *.o a.out check.out gmon.out leak.log casket* *.tct *.idx.* *~


static :
	make LDFLAGS="$(LDFLAGS) -static"


.PHONY : all clean static tctchat.tct tctsearch.tct



#================================================================
# Building binaries
#================================================================


tcutilex : tcutilex.o
	$(LDENV) $(CC) $(CFLAGS) -o $@ $< $(LDFLAGS) $(LIBS)


tchdbex : tchdbex.o
	$(LDENV) $(CC) $(CFLAGS) -o $@ $< $(LDFLAGS) $(LIBS)


tcbdbex : tcbdbex.o
	$(LDENV) $(CC) $(CFLAGS) -o $@ $< $(LDFLAGS) $(LIBS)


tcfdbex : tcfdbex.o
	$(LDENV) $(CC) $(CFLAGS) -o $@ $< $(LDFLAGS) $(LIBS)


tctdbex : tctdbex.o
	$(LDENV) $(CC) $(CFLAGS) -o $@ $< $(LDFLAGS) $(LIBS)


tcadbex : tcadbex.o
	$(LDENV) $(CC) $(CFLAGS) -o $@ $< $(LDFLAGS) $(LIBS)


tctchat.cgi : tctchat.c
	$(LDENV) $(CC) $(C99FLAGS) -o $@ $< $(LDFLAGS) $(LIBS)


tctsearch.cgi : tctsearch.c
	$(LDENV) $(CC) $(C99FLAGS) -o $@ $< $(LDFLAGS) $(LIBS)


tctchat.tct :
	rm -rf tctchat.tct*
	LD_LIBRARY_PATH=/usr/local/lib:.:.. ../tctmgr create tctchat.tct
	LD_LIBRARY_PATH=/usr/local/lib:.:.. ../tctmgr setindex -it qgram tctchat.tct "t"
	chmod 666 tctchat.tct*


tctsearch.tct :
	rm -rf tctsearch.tct*
	LD_LIBRARY_PATH=/usr/local/lib:.:.. ../tctmgr create tctsearch.tct
	LD_LIBRARY_PATH=/usr/local/lib:.:.. ../tctmgr setindex -it qgram tctsearch.tct "text"
	LD_LIBRARY_PATH=/usr/local/lib:.:.. ../tctmgr setindex -it qgram tctsearch.tct "title"
	../lab/htmltotsv .. | \
	  LD_LIBRARY_PATH=/usr/local/lib:.:.. ../tctmgr importtsv tctsearch.tct



# END OF FILE
