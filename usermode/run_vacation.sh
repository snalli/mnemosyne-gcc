#!/bin/bash
# sudo to enable trace markers, -e enables tracing
bin=./build/bench/stamp-kozy/vacation/vacation
action=$1

if [[ $action == '-h' ]]
then
	$bin -h
else

	$bin -c4 -r4000000 -t2000000 -n1 -q80 -u99 -e1
fi

