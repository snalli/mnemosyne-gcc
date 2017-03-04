#!/bin/bash
# Reduce the number of relations 'r' if you don't have space
# This amounts to roughly 1.5 GB of reservation tables
bin=./build/bench/stamp-kozy/vacation/vacation
action=$1

if [[ $action == '-h' ]]
then
	$bin -h
else

	$bin -c0 -r4000000 -n1 -q100 -u100
fi

