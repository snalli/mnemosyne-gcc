#!/bin/bash

mountpoint="/mnt/pcmfs"
pcmdevice0="/dev/pcm0"
pcmctrldevice0="/dev/pcm0-ctrl"
objdir="build"

function mkfs {
	if [ ! -b "$pcmdevice0" ]
	then 
	mknod $pcmdevice0 b 240 0
	fi
	if [ ! -c "$pcmctrldevice0" ]
	then 
	mknod $pcmctldevice0 c 241 0
	chmod a+wr $pcmctldevice0
	fi
	/sbin/insmod $objdir/pcmdisk.ko
	/sbin/mke2fs -m 0 /dev/pcm0
	if [ ! -d "$mountpoint" ]
	then 
	mkdir $mountpoint
	fi
	mount /dev/pcm0 /mnt/pcmfs -o noatime,nodiratime
	chmod a+wr /mnt/pcmfs
}

function rmfs {
	umount $mountpoint
	/sbin/rmmod pcmdisk.ko
}

if [ "$1" = "-c" ]
then
mkfs
fi

if [ "$1" = "-r" ]
then
rmfs
fi
