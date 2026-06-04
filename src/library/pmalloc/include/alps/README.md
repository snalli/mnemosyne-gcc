# ALPS: Allocator Layers for Persistent Shared memory

![Alps](doc/figures/alps-logo.png)

ALPS provides a collection of low-level abstraction layers that relief the
user from the details of mapping, addressing, and allocating persistent 
shared memory.
These layers can be used as building blocks for building higher-level 
memory allocators such as persistent heaps.
Shared persistent memory refers to non-volatile memory shared among
multiple compute nodes and can take different forms, such as
disaggregated non-volatile memory accessible over a fabric (also
known as fabric-attached memory or FAM) or multi-socket non-volatile
memory.

## Supported Platforms

ALPS only supports Linux at the moment.

## Quick Start Guide

This section provides a quick introduction to setting up and testing
ALPS on a CC-NUMA machine.
We assume the ALPS source code is already deployed in directory $ALPS.

1. Change into the ALPS source directory:

 ```
 $ cd $ALPS
 ```

2. Install dependencies:

 ```
 $ ./install-dep
 ```

3. Build ALPS

 ```
 $ mkdir build
 $ cd build
 $ cmake .. -DTARGET_ARCH_MEM=CC-NUMA
 $ make
 ```

4. Run unit tests against:

 ```
 ctest
 ```

## Installation

Instructions for building and installing ALPS on different platforms and
environments is available on a platform by platform basis:

* [CC-NUMA](INSTALL-NUMA.md): Linux platform on Cache-Coherent Non-Uniform Memory
Access (CC-NUMA) architecture

## Example Programs

Alps comes with several samples in the `examples` directory.

## Style Guide 

We follow the Google C++ style guide available here:

https://google.github.io/styleguide/cppguide.html
