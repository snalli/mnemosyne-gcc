# Mnemosyne

### Mnemosyne: Lightweight Persistent Memory (PM)

Mnemosyne provides a simple interface for programming with persistent 
memory. Programmers declare global persistent data with the keyword 
persistent or allocate it dynamically. Mnemosyne provides primitives for 
directly modifying persistent variables and supports consistent updates 
through a lightweight transaction mechanism. 

### Authors:

* Haris Volos   <hvolos@cs.wisc.edu>
* Andres Jaan Tack   <tack@cs.wisc.edu>
* Sanketh Nalli <nalli@wisc.edu>

### Dependencies:

* SCons: A software construction tool
* Preferable, GCC 6.2.1 and above 
* libconfig
```
	Fedora : $ dnf install libconfig-devel.x86_64 libconfig.x86_64
	Ubuntu : $ apt-get install libconfig-dev libconfig9
```
* gelf
```
	Fedora : $ dnf install elfutils-libelf-devel.x86_64 elfutils-libelf.x86_64
	Ubuntu : $ apt-get install libelf-dev elfutils
```
* libevent (For memcached)
```
	Fedora : $ dnf install libevent-devel.x86_64 
	Ubuntu : $ apt-get install libevent-dev
```
* /dev/shm or mount point backed by persistent memory
	- The heap will be placed in segments_dir defined in mnemosyne.ini
	- Please ensure you have at least 1.00 GB of space for the heap.

* ALPS persistent memory allocator
```
ALPS Dependencies (on Ubuntu) :
	cmake
    	libattr1-dev
    	libboost-all-dev
    	libevent-dev
    	libnuma1
    	libnuma-dev
    	libyaml-cpp-dev
	
Install the above dependencies and then type the commands below
at your terminal. Please find equivalent packages for your 
flavor of Linux. 

	$ cd usermode/library/pmalloc/include/alps
	$ mkdir build
	$ cd build
	$ cmake .. -DTARGET_ARCH_MEM=CC-NUMA -DCMAKE_BUILD_TYPE=Release
	$ make
	
Learn more here on how to compile alps : 
github.com/snalli/mnemosyne-gcc/tree/alps/usermode/library/pmalloc/include/alps
```

### Build Mnemosyne:
```
$ cd usermode
$ scons [--build-stats] [--config-ftrace] [--verbose]
 
* scons -h <For more options>
```

### Run a simple example:

* Simple example
	- Read and modify value of persistent flag across executions
	- Test pmalloc - persistent memory allocator
	- Test concurrent reader and writer
```
$ cd usermode
$ scons --build-example=simple
$ ./build/examples/simple/simple 
persistent flag: 0 --> 1
&persistent flag = 0x100020038000

starting pmalloc bench
persistent ptr =0x100ba0035fe0, sz=32
persistent ptr & cl_mask = 0x100ba0035fc0
(WRITER) persistent ptr =0x100ba0035fe0, sz=32
(READER) persistent ptr =0x100ba0035fe0, sz=32

$ ./build/examples/simple/simple 
persistent flag: 1 --> 0
&persistent flag = 0x100020038000

starting pmalloc bench
persistent ptr =0x100ba0035fc0, sz=32
persistent ptr & cl_mask = 0x100ba0035fc0
(WRITER) persistent ptr =0x100ba0035fc0, sz=32
(READER) persistent ptr =0x100ba0035fc0, sz=32
```

### Run a complex benchmark:

### Build Vacation:
```
$ cd usermode
$ scons --build-bench=stamp-kozy [--verbose]
```

### Initialize Vacation:
```
$ cd usermode
$ export LD_LIBRARY_PATH=`pwd`/library:$LD_LIBRARY_PATH
$ ./build/bench/stamp-kozy/vacation/vacation -c0 -n1 -r65536 -q100
```

### Run Vacation:
```
$ cd usermode
$ export LD_LIBRARY_PATH=`pwd`/library:$LD_LIBRARY_PATH
$ ./build/bench/stamp-kozy/vacation/vacation -t1000 -c2 -n10 -r65536 -q90 -u100

    Table Index         = RBTREE
    Transactions        = 1000
    Clients             = 2
    Transactions/client = 500
    Queries/transaction = 10
    Relations           = 65536
    Query percent       = 90
    Query range         = 58982
    Percent user        = 100
    Enable trace        = 0
Initializing manager... 

***************************************************

Re-using tables from previous incarnation...

Persistent table pointers.

Car Table      = 0x100ba0035ff0
Room Table     = 0x100ba0035fe0
Flight Table   = 0x100ba0035fd0
Customer Table = 0x100ba0035fc0

***************************************************
Initializing clients... done.

Running clients...

Starting thread 9201
Thread-0 finished 500 queries
Starting thread 9203
Thread-1 finished 500 queries
done.Time = 0.127621
Deallocating memory... done.
```
### Build Memcached:
```
$ scons --build-bench=memcached  [--verbose]
* Check run_*.sh scripts to learn more on how to run memcached.
```

### Documentation:
For further information please refer to the Doxygen generated documentation.
Running doxygen will create documentation under mnemosyne/doc/html

$ cd ./usermode/..
$ doxygen

### License:

GPL-V2
See license file under each module

