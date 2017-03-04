#!/bin/bash
bin=./build/bench/memcached/memcached-1.2.4-mtm/memcached
action=$1

if [[ $action == '-h' ]]
then
	$bin -h
else

#scons --build-bench=memcached
#ldd ./build/bench/memcached/memcached-1.2.4-mtm/memcached
./build/bench/memcached/memcached-1.2.4-mtm/memcached -u root -p 11211 -l 127.0.0.1 -t 4 -x
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

fi
