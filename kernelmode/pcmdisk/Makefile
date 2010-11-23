# build modules
obj-m = pcmdisk.o
pcmdisk-objs = brd.o pcm.o

# use the kernel build system
KERNEL_VERSION := `uname -r`
KERNEL_SOURCE := /lib/modules/$(KERNEL_VERSION)/build

SRCDIR=`pwd`
OBJDIR=`pwd`/build

all: create_build_dir build_kernelmodule build_userapp

clean: clean_kernelmodule clean_userapp

create_build_dir:
	if test ! -d $(OBJDIR); then                             \
		mkdir $(OBJDIR);                                 \
		cp Makefile $(OBJDIR)/Makefile;                  \
		ln -s $(SRCDIR)/brd.c $(OBJDIR)/brd.c;           \
		ln -s $(SRCDIR)/pcm.c $(OBJDIR)/pcm.c;           \
		ln -s $(SRCDIR)/pcm.h $(OBJDIR)/pcm.h;           \
		ln -s $(SRCDIR)/userctrl.c $(OBJDIR)/userctrl.c; \
	fi

build_kernelmodule: 
	make -C $(KERNEL_SOURCE)  M=$(OBJDIR) modules

clean_kernelmodule:
	make -C $(KERNEL_SOURCE) M=$(OBJDIR) clean

build_userapp:
	gcc $(OBJDIR)/userctrl.c -o $(OBJDIR)/userctrl

clean_userapp:
	rm $(OBJDIR)/userctrl
