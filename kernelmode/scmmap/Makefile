obj-m += scmmap.o
scmmap-objs := main_kernel.o index.o

PWD    := $(shell pwd)
KDIR    := $(PWD)/../linux-2.6.33

all: kernelmodule user

kernelmodule:
	make -C $(KDIR) M=$(PWD) modules

user: main_user.c
	gcc -g main_user.c -o scmmap_user

clean:
	make -C $(KDIR) M=$(PWD) clean
	rm scmmap_user

