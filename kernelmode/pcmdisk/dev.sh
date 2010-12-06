#!/bin/bash

mountpoint="/mnt/pcmfs"
pcmdevice0="/dev/pcm0"
pcmctldevice0="/dev/pcm0-ctl"
objdir="build"

mknod $pcmctldevice0 c 241 0
chmod a+wr $pcmctldevice0
