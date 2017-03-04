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

elif [ "$var" == "--memcached" ]
then
    echo "Look at README"
#    echo "Starting memcached server"
#    server_bin=./build/bench/memcached/memcached-1.2.4-mtm/memcached
#    $server_bin -u root -p 11211 -l 127.0.0.1 -t 4 -x &
#    sleep 2
#	if [ "$action" == "--small" ]
#	then
#        MEMCACHED_RUN_CNF="$PWD/run.cnf"
#        MEMASLAP_BIN=$PWD/bench/memcached/memslap
#        MEMASLAP_THREADS="1"  
#        MEMASLAP_CONCURRENCY="1" 
#        VALUE_SIZE_ARR=( 64 128 256 512 )
#        SERVER_IP="127.0.0.1"
#        SERVER_PORT=11211
#        idx=0
#        for (( idx=0; idx<${#VALUE_SIZE_ARR[@]}; idx++ ))
#        do
#            val_size=${VALUE_SIZE_ARR[$idx]}
#            num_ops=1000
#            run_time="1s"
#            $MEMASLAP_BIN -s $SERVER_IP:$SERVER_PORT -c $MEMASLAP_CONCURRENCY -x $num_ops -T $MEMASLAP_THREADS -X $val_size -F $MEMCACHED_RUN_CNF -d 1
#            sleep 1
#        done
#	elif [ "$action" == "--med" ]
#	then
#        MEMCACHED_RUN_CNF="$PWD/run.cnf"
#        MEMASLAP_BIN=$PWD/bench/memcached/memslap
#        MEMASLAP_THREADS="2"  
#        MEMASLAP_CONCURRENCY="2" 
#        VALUE_SIZE_ARR=( 64 128 256 512 )
#        SERVER_IP="127.0.0.1"
#        SERVER_PORT=11211
#        idx=0
#        for (( idx=0; idx<${#VALUE_SIZE_ARR[@]}; idx++ ))
#        do
#            val_size=${VALUE_SIZE_ARR[$idx]}
#            num_ops=10000
#            run_time="10s"
#            $MEMASLAP_BIN -s $SERVER_IP:$SERVER_PORT -c $MEMASLAP_CONCURRENCY -x $num_ops -T $MEMASLAP_THREADS -X $val_size -F $MEMCACHED_RUN_CNF -d 1
#            sleep 1
#        done
#
#	elif [ "$action" == "--large" ]
#	then
#        MEMCACHED_RUN_CNF="$PWD/run.cnf"
#        MEMASLAP_BIN=$PWD/bench/memcached/memslap
#        MEMASLAP_THREADS="4"  
#        MEMASLAP_CONCURRENCY="4" 
#        VALUE_SIZE_ARR=( 64 128 256 512 )
#        SERVER_IP="127.0.0.1"
#        SERVER_PORT=11211
#        idx=0
#        for (( idx=0; idx<${#VALUE_SIZE_ARR[@]}; idx++ ))
#        do
#            val_size=${VALUE_SIZE_ARR[$idx]}
#            num_ops=100000
#            run_time="100s"
#            $MEMASLAP_BIN -s $SERVER_IP:$SERVER_PORT -c $MEMASLAP_CONCURRENCY -x $num_ops -T $MEMASLAP_THREADS -X $val_size -F $MEMCACHED_RUN_CNF -d 1
#            sleep 1
#        done
#
#    elif [ "$action" == "-h" ]
#    then
#        $bin -h
#	fi
else
	echo "Invalid workload $var, choose from vacation, memcached"
fi
