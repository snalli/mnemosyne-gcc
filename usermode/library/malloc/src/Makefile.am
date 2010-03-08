##
## The Hoard Multiprocessor Memory Allocator
## <http://www.hoard.org>
##
## Contact author: Emery Berger, http://www.cs.utexas.edu/users/emery
##
## Copyright (c) 1998 - 2001, The University of Texas at Austin.
##
## This library is free software; you can redistribute it and/or modify
## it under the terms of the GNU Library General Public License as
## published by the Free Software Foundation, http://www.fsf.org.
##
## This library is distributed in the hope that it will be useful, but
## WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
## Library General Public License for more details.
##

# Makefile.am for Hoard-2.1

VPATH  = @srcdir@

# lib_LIBRARIES = libhoard.a
lib_LTLIBRARIES = libhoard.la
# noinst_PROGRAMS = testmymalloc-hoard testmymalloc threadtest threadtest-hoard cache-scratch cache-scratch-hoard

if USE_WORKSHOP
arch-specific.lo: sparc-interchange.il arch-specific.cpp 
	$(LTCXXCOMPILE) -c -o $@ $^
workshop_objects = arch-specific.lo
else
ARCH_SPECIFIC_SRC = arch-specific.cpp
endif

libhoard_la_SOURCES = $(ARCH_SPECIFIC_SRC) mallocwrap.cpp threadheap.cpp processheap.cpp superblock.cpp wrapper.cpp hoardheap.cpp dlmalloc.c
EXTRA_libhoard_la_SOURCES = arch-specific.cpp
libhoard_la_LIBADD = $(workshop_objects)

# CURRENT (interface) : REVISION (implementation) : AGE (how many interfaces...)
libhoard_la_LDFLAGS = -avoid-version -module

#testmymalloc_SOURCES = benchmarks/testmymalloc/testmymalloc.cpp
#testmymalloc_LDADD = @LIBTHREAD@ @LIBSCHED@

#testmymalloc_hoard_SOURCES = benchmarks/testmymalloc/testmymalloc.cpp
#testmymalloc_hoard_LDADD = libhoard.a @LIBTHREAD@ @LIBSCHED@

#threadtest_SOURCES = benchmarks/threadtest/threadtest.cpp arch-specific.cpp dlmalloc.c
#threadtest_LDADD = @LIBTHREAD@ @LIBSCHED@

#threadtest_hoard_SOURCES = benchmarks/threadtest/threadtest.cpp
#threadtest_hoard_LDADD = libhoard.a @LIBTHREAD@ @LIBSCHED@

#cache_scratch_SOURCES = benchmarks/cache-scratch/cache-scratch.cpp arch-specific.cpp dlmalloc.c
#cache_scratch_LDADD = @LIBTHREAD@ @LIBSCHED@

#cache_scratch_hoard_SOURCES = benchmarks/cache-scratch/cache-scratch.cpp
#cache_scratch_hoard_LDADD = libhoard.a @LIBTHREAD@ @LIBSCHED@

