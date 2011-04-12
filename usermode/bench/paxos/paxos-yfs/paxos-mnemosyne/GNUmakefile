MNEMOSYNE_SRC=/home/hvolos/workspace/mnemosyne/usermode/library
#MNEMOSYNE_BUILD=/home/hvolos/workspace/mnemosyne.deploy/deployments/pcm-150_async-trunc_tmlog-tornbit/build
MNEMOSYNE_BUILD=/home/hvolos/tmp/async/build/
RPC=./rpc
MNEMOSYNE_CXXFLAGS=-DM_PCM_CPUFREQ=2500 -DM_PCM_EMULATE_LATENCY -DM_PCM_LATENCY_WRITE=150 -DM_PCM_BANDWIDTH_MB=5000 -DRAM_SYSTEM_PEAK_BANDWIDTH_MB=7000
CXXFLAGS =  -O2 -g -MD -Wall -I. -I$(RPC) -I$(MNEMOSYNE_SRC)/mnemosyne/include -I$(MNEMOSYNE_SRC)/common -D_FILE_OFFSET_BITS=64 $(MNEMOSYNE_CXXFLAGS) 
LDFLAGS = -L. -L/usr/local/lib
LDLIBS = -lpthread 
LDLIBS += $(shell test -f `gcc -print-file-name=librt.so` && echo -lrt)
LDLIBS += $(shell test -f `gcc -print-file-name=libdl.so` && echo -ldl)
LDLIBS += $(MNEMOSYNE_BUILD)/library/mcore/libmcore.so
CC = g++
CXX = g++

hfiles1=rpc/fifo.h rpc/connection.h rpc/rpc.h rpc/marshall.h rpc/method_thread.h\
	rpc/thr_pool.h rpc/pollmgr.h rpc/jsl_log.h rpc/slock.h rpc/rpctest.cc\
	gettime.h gettime.cc
hfiles4=log_paxos.h driver.h paxos.h paxos_protocol.h
paxos_files = paxos.cc driver.cc log_paxos.cc handle.cc log_tornbit.cc


all: driver_server

rpclib=rpc/rpc.cc rpc/connection.cc rpc/pollmgr.cc rpc/thr_pool.cc rpc/jsl_log.cc gettime.cc
rpc/librpc.a: $(patsubst %.cc,%.o,$(rpclib))
	rm -f $@
	ar cq $@ $^
	ranlib rpc/librpc.a

rpc/rpctest=rpc/rpctest.cc
rpc/rpctest: $(patsubst %.cc,%.o,$(rpctest)) rpc/librpc.a

driver_server= driver_main.cc
driver_server+= $(paxos_files)
driver_server : $(patsubst %.cc,%.o,$(driver_server)) rpc/librpc.a
	$(CXX) $^ $(LDLIBS) -o $@

%.o: %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

-include *.d

.PHONY : clean
clean : 
	rm -rf rpc/rpctest rpc/*.o rpc/*.d rpc/librpc.a *.o *.d driver_server
