#!/bin/bash

mountpoint="/mnt/pcmfs"
pcmdevice0="/dev/pcm0"

function mkfs {
	if [ ! -b "$pcmdevice0" ]
	then 
	mknod $pcmdevice0 b 240 0
	fi
	/sbin/insmod pcmdisk.ko
	/sbin/mke2fs -m 0 /dev/pcm0
	if [ ! -d "$mountpoint" ]
	then 
	mkdir $mountpoint
	fi
	mount /dev/pcm0 /mnt/pcmfs
	chmod a+wr /mnt/pcmfs
}

function rmfs {
	umount $mountpoint
	/sbin/rmmod pcmdisk.ko
}

if [ "$1" = "-m" ]
then
mkfs
fi

if [ "$1" = "-r" ]
then
rmfs
fi
