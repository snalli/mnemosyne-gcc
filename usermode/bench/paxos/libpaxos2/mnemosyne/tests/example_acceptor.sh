#!/bin/bash

export LD_LIBRARY_PATH=/scratch/local/lib/icc/intel64 
cd /afs/cs.wisc.edu/p/multifacet/users/hvolos/workspace/mnemosyne/mnemosyne/bench/paxos/libpaxos2/tests
gdb ./example_acceptor
