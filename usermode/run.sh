#!/bin/bash
action=$1
var=$2
trace=$3

export LD_LIBRARY_PATH=$PWD/library/:$LD_LIBRARY_PATH

if [ "$action" == "-h" ] && [ "$var" == "" ]
then
    echo 'Help is workload-specific.'
    echo 'Try ./run.sh -h --vacation'
	exit
fi

if [ "$trace" == "--trace" ]
then
    trace=" -e1" 
else
    trace=" "
fi

if [ "$var" == "--vacation" ]
then
    bin=./build/bench/stamp-kozy/vacation/vacation
	if [ "$action" == "--small" ]
	then
        $bin -c4 -r4000 -t2000 -n1 -q80 -u99 $trace 
	elif [ "$action" == "--med" ]
	then
        $bin -c4 -r40000 -t20000 -n1 -q80 -u99 $trace
	elif [ "$action" == "--large" ]
	then
        $bin -c4 -r4000000 -t2000000 -n1 -q80 -u99 $trace
    elif [ "$action" == "-h" ]
    then
        $bin -h
    fi
elif [[ $var == '--memcached' ]]
then
	echo "run server, then client. check scripts."
else
	echo "Invalid workload $var, choose from vacation, memcached"
fi
