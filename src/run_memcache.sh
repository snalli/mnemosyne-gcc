#!/bin/bash
killall memcached
bin=./build/bench/memcached/memcached-1.2.4-mtm/memcached

if [[ $1 == '-h' ]]
then
	$bin -h
	exit
elif [[ $1 == '--trace' ]]
then
	trace="-x"
else
	trace=""
fi

#scons --build-bench=memcached
#ldd ./build/bench/memcached/memcached-1.2.4-mtm/memcached
./build/bench/memcached/memcached-1.2.4-mtm/memcached -u root -p 11211 -l 127.0.0.1 -t 4 $trace &
#./build/bench/memcached/memcached-1.2.4-mtm/memcached -u root -p 11211 -l 127.0.0.1 -t 4
#./build/bench/memcached/memcached-1.2.4-mtm/memcached -u root -p 11211 -l 127.0.0.1 -t 1 
#./build/bench/memcached/memcached-1.2.4-mtm/memcached -u root -p 11211 -l 127.0.0.1 -t 1 -vv
#./build/bench/memcached/memcached-1.2.4-mtm/memcached -u root -p 11211 -l 127.0.0.1 -t 1 -X 2 -T 5 -O 2 -vv
#./build/bench/memcached/memcached-1.2.4-mtm/memcached -u root -p 11211 -l 127.0.0.1 -t 1 -f 2 -vv
#./build/bench/memcached/memcached-1.2.4-mtm/memcached -u root -p 11211 -l 127.0.0.1 -t 1 -m 16 -vv
#strace ./build/bench/memcached/memcached-1.2.4-mtm/memcached -u root -p 11211 -l 127.0.0.1 -t 1 
#strace ./build/bench/memcached/memcached-1.2.4-mtm/memcached -u root -p 11211 -l 127.0.0.1 -t 1 -vv
#gdb --args ./build/bench/memcached/memcached-1.2.4-mtm/memcached -u root -p 11211 -l 127.0.0.1 -t 1 -vv
#objdump -D ./build/bench/memcached/memcached-1.2.4-mtm/memcached

