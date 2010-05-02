efw       : A framework for deploying, running, and analyzing experiments
kernelmode: Mnemosyne kernel extensions
usermode  : Mnemosyne user-mode library suite

Mnemosyne Persistent Memory Suite
=================================
This library suite is designed to help programmers leverage byte-addressible persistent memory in their programs. It is currently under development.

Overview
--------
Currently, Mnemosyne is build in two separable libraries:

1. **`libmnemosyne`** supports raw byte-addressible memory regions. You can declare global variables to be _persistent_, and the values stored in these variables will be reincarnated on every subsequent run of the same program.
2. **`libmtm`** adds transactional safety guarantees to your persistent memory regions. Accesses to persistent memory, when wrapped in transactions, are guaranteed in their atomicity. That is, the changes either _all_ occur, or _none of them do,_ so if someone pulls the plug on your machine, you may be guaranteed that data structures are not corrected. This library requires a transactional compiler conforming to the [Intel STM Prototype ABI](http://software.intel.com/en-us/articles/intel-c-stm-compiler-prototype-edition-20/#ABI).

Dependencies
------------
The following _must_ be installed to successfully compile the libraries. Compilation has been tested with GCC 4.1 and up.

- DEPRECATED: [`libatomic_ops`](http://www.hpl.hp.com/research/linux/atomic_ops/) library (e.g. `libatomic_ops-devel-1.2-2.el5.x86_64.rpm`)
  There has been a report of a bug in the CAS implementation which was only fixed quite recently (as of February 2010). We now use a light version of the atomic-ops library which is integrated into our source tree.
- [`libconfig`](www.hyperrealm.com/libconfig/) (yum install libconfig libconfig-devel)
- [UnitTest++](unittest-cpp.sourceforge.net) (`libunittest++-dev` in most RPM systems)

Testing
-------
Unit tests are intended to run as part of the compilation process (thus making it hard to check in broken code). 
