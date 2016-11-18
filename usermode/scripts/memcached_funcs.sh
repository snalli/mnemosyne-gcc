#!/bin/bash -x

MEMCACHED_DIR="/u/s/a/sankey/Desktop/mnemosyne-gcc/usermode"
export LD_LIBRARY_PATH=$MEMCACHED_DIR/library

MEMCACHED_LOAD_CNF="$MEMCACHED_DIR/scripts/load.cnf"
MEMCACHED_RUN_CNF="$MEMCACHED_DIR/scripts/run.cnf"

MEMASLAP_BIN="/u/s/a/sankey/Desktop/mnemosyne-gcc/usermode/memslap"
MEMASLAP_KEY_SIZE="16" # i
MEMASLAP_THREADS="16"  # i
MEMASLAP_CONCURRENCY="32" # i
MEMASLAP_UDP=""

### New values following distribution in facebook's paper
VALUE_SIZE_ARR=( 16 32 64 128 256 512 )
# VALUE_SIZE_ARR=( 16 32 64 128 256 512 1024 2048 4096 8192 )
# alignment requirements : going to report using top and the memory consumed
# by memchaced process
NUM_OPS_ARR=( 20132864 10066944 10066944 35232768 25165824 3145728 1888256 2359296 786432 1022976 )
# change the ratio of ?
# exact num of ops
LIMIT_OPS_ARR=( 15728640 7864320 5242880 8388608 12582912 1048576 524288 524288 367616 157696 )

SERVER_IP="127.0.0.1"
#SERVER_IP="192.168.254.101"
SERVER_PORT=11211
SERVER_NUM_THREADS=8

function get_server_list () {
	local sharded=$1
	local server_list="$SERVER_IP:$SERVER_PORT"
	if [ $sharded -eq 1 ]; then
		for (( i=1; i<$SERVER_NUM_THREADS; i++ ))
		do
			local port=`expr $SERVER_PORT + $i`
			server_list=$server_list",$SERVER_IP:"$port
		done
	fi
	echo "$server_list"
}

function load_memcached_dataset () {
	local sharded=$1
	local server_list=$(get_server_list $sharded)

	local idx=0
	for (( idx=0; idx<${#VALUE_SIZE_ARR[@]}; idx++ ))
	do
		local val_size=${VALUE_SIZE_ARR[$idx]}
		# local num_ops=${NUM_OPS_ARR[$idx]}

		$MEMASLAP_BIN -s $server_list -c 1 -x 1000 $MEMASLAP_UDP -T 1 -X $val_size -F $MEMCACHED_LOAD_CNF 
		sleep 1
		#$MEMASLAP_BIN -s $server_list -k $MEMASLAP_KEY_SIZE -c $MEMASLAP_CONCURRENCY -x $num_ops $MEMASLAP_UDP -T $MEMASLAP_THREADS -X $val_size -F $MEMCACHED_LOAD_CNF -l
	done
}

function run_memcached_test () {
	local sharded=$1
	local server_list=$(get_server_list $sharded)
	local run_time="20s"

	local idx=0
	for (( idx=0; idx<${#VALUE_SIZE_ARR[@]}; idx++ ))
	do
		local val_size=${VALUE_SIZE_ARR[$idx]}
		# local num_ops=${NUM_OPS_ARR[$idx]}
		sleep 1
		#$MEMASLAP_BIN -s $server_list -k $MEMASLAP_KEY_SIZE -c $MEMASLAP_CONCURRENCY -x $num_ops $MEMASLAP_UDP -T $MEMASLAP_THREADS -X $val_size -F $MEMCACHED_RUN_CNF -d 1024 -t $run_time -r
		#sleep 1
	done
}

function run_fixed_memcached_test () {
	local sharded=$1
	local server_list=$(get_server_list $sharded)
	local run_time="20s"

	local idx=0
	for (( idx=0; idx<${#VALUE_SIZE_ARR[@]}; idx++ ))
	do
		local val_size=${VALUE_SIZE_ARR[$idx]}
		local num_ops=50000
		# local limit_ops=${LIMIT_OPS_ARR[$idx]}
		# 95% puts and 5% gets
		$MEMASLAP_BIN -s $server_list -c 4 -x $num_ops $MEMASLAP_UDP -T 4 -X $val_size -F $MEMCACHED_RUN_CNF -d 1
		#$MEMASLAP_BIN -s $server_list -k $MEMASLAP_KEY_SIZE -c $MEMASLAP_CONCURRENCY -x $num_ops -L $limit_ops -T $MEMASLAP_THREADS -X $val_size -F $MEMCACHED_RUN_CNF -d 1024 -r
	done
}

