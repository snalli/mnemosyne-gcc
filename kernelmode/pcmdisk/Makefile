# build modules
obj-m = pcmdisk.o
pcmdisk-objs = brd.o pcm.o

# use the kernel build system
KERNEL_VERSION := `uname -r`
KERNEL_SOURCE := /lib/modules/$(KERNEL_VERSION)/build

all: build_kernelmodule build_userapp

clean: clean_kernelmodule clean_userapp



build_kernelmodule:
	make -C $(KERNEL_SOURCE) M=`pwd` modules
clean_kernelmodule:
	make -C $(KERNEL_SOURCE) M=`pwd` clean

build_userapp:
	gcc user_control.c -o user_control

clean_userapp:
	rm user_control
