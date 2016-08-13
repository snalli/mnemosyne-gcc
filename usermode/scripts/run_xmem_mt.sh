#!/bin/bash

MEMCACHED_DIR="/u/s/a/sankey/Desktop/mnemosyne/usermode/scripts"

source "$MEMCACHED_DIR/memcached_funcs.sh"

echo "Running multi-threaded dataset ..."
#run_memcached_test 0
run_fixed_memcached_test 0
# single process mt server and not sharded
