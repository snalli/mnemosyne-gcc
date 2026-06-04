#!/bin/bash
# Reduce the number of relations 'r' if you don't have space
# This amounts to roughly 1.5 GB of reservation tables
bin=./build/bench/stamp-kozy/vacation/vacation
#dbg_tool="strace -ff -e trace=lseek"
#dbg_tool="gdb --args "
action=$1

if [[ $action == '-h' ]]
then
	$bin -h
else

	$dbg_tool $bin -c0 -r400 -n1 -q100 -u100
fi

