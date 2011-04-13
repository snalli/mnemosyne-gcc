// Allocator that wraps operator new -*- C++ -*-

// Copyright (C) 2001, 2002, 2003, 2004, 2005, 2009
// Free Software Foundation, Inc.
//
// This file is part of the GNU ISO C++ Library.  This library is free
// software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the
// Free Software Foundation; either version 3, or (at your option)
// any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// Under Section 7 of GPL version 3, you are granted additional
// permissions described in the GCC Runtime Library Exception, version
// 3.1, as published by the Free Software Foundation.

// You should have received a copy of the GNU General Public License and
// a copy of the GCC Runtime Library Exception along with this program;
// see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
// <http://www.gnu.org/licenses/>.

/** @file ext/new_allocator.h
 *  This file is a GNU extension to the Standard C++ Library.
 */

#ifndef _NEW_ALLOCATOR_H
#define _NEW_ALLOCATOR_H 1

#include <new>
#include <bits/functexcept.h>
#include <bits/move.h>
#include "pmalloc.h"

__attribute__((tm_callable))
inline void* operator new(size_t block_size) {
	void* block = pmalloc(block_size);
	if (block == 0)
		throw std::bad_alloc();
	else
		return block;
}

__attribute__((tm_callable))
inline void operator delete(void* block) {
	pfree(block);
}

_GLIBCXX_BEGIN_NAMESPACE(__gnu_cxx)

  using std::size_t;
  using std::ptrdiff_t;

  /**
   *  @brief  An allocator that uses global new, as per [20.4].
   *  @ingroup allocators
   *
   *  This is precisely the allocator defined in the C++ Standard. 
   *    - all allocation calls operator new
   *    - all deallocation calls operator delete
   */
  template<typename _Tp>
    class new_allocator
    {
    public:
      typedef size_t     size_type;
      typedef ptrdiff_t  difference_type;
      typedef _Tp*       pointer;
      typedef const _Tp* const_pointer;
      typedef _Tp&       reference;
      typedef const _Tp& const_reference;
      typedef _Tp        value_type;

      template<typename _Tp1>
        struct rebind
        { typedef new_allocator<_Tp1> other; };

      __attribute__((tm_callable))
      new_allocator() throw() { }

	__attribute((tm_callable))
      new_allocator(const new_allocator&) throw() { }

      template<typename _Tp1>
      __attribute__((tm_callable))
        new_allocator(const new_allocator<_Tp1>&) throw() { }

		__attribute__((tm_callable))
      ~new_allocator() throw() { }

      pointer
      address(reference __x) const { return &__x; }

      const_pointer
      address(const_reference __x) const { return &__x; }

      // NB: __n is permitted to be 0.  The C++ standard says nothing
      // about what the return value is when __n == 0.
      __attribute__((tm_callable))
      pointer
      allocate(size_type __n, const void* = 0)
      { 
	if (__builtin_expect(__n > this->max_size(), false))
	  std::__throw_bad_alloc();

	return static_cast<_Tp*>(::operator new(__n * sizeof(_Tp)));
      }

      // __p is not permitted to be a null pointer.
      __attribute__((tm_callable))
      void
      deallocate(pointer __p, size_type)
      { ::operator delete(__p); }

	__attribute__((tm_pure))
      size_type
      max_size() const throw() 
      { return size_t(-1) / sizeof(_Tp); }

      // _GLIBCXX_RESOLVE_LIB_DEFECTS
      // 402. wrong new expression in [some_] allocator::construct
      __attribute__((tm_callable))
      void 
      construct(pointer __p, const _Tp& __val) 
      { ::new((void *)__p) _Tp(__val); }

#ifdef __GXX_EXPERIMENTAL_CXX0X__
      template<typename... _Args>
        void
        construct(pointer __p, _Args&&... __args)
	{ ::new((void *)__p) _Tp(std::forward<_Args>(__args)...); }
#endif

	__attribute__((tm_callable))
      void 
      destroy(pointer __p) { __p->~_Tp(); }
    };

  template<typename _Tp>
    inline bool
    operator==(const new_allocator<_Tp>&, const new_allocator<_Tp>&)
    { return true; }
  
  template<typename _Tp>
    inline bool
    operator!=(const new_allocator<_Tp>&, const new_allocator<_Tp>&)
    { return false; }

_GLIBCXX_END_NAMESPACE

#endif
