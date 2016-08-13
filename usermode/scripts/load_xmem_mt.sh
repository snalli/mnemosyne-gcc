#!/bin/bash

MEMCACHED_DIR="/u/s/a/sankey/Desktop/mnemosyne/usermode/scripts"

source "$MEMCACHED_DIR/memcached_funcs.sh"

echo "Loading multi-threaded dataset ..."
load_memcached_dataset 0

