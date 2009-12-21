/* Copyright (C) 2009 Free Software Foundation, Inc.
   Contributed by Richard Henderson <rth@redhat.com>.

   This file is part of the GNU Transactional Memory Library (libitm).

   Libitm is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   Libitm is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
   FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
   more details.

   Under Section 7 of GPL version 3, you are granted additional
   permissions described in the GCC Runtime Library Exception, version
   3.1, as published by the Free Software Foundation.

   You should have received a copy of the GNU General Public License and
   a copy of the GCC Runtime Library Exception along with this program;
   see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
   <http://www.gnu.org/licenses/>.  */

/* Implements an AA tree (http://en.wikipedia.org/wiki/AA_tree) with an
   integer key, and data attached to the node via flexible array member.  */

typedef uintptr_t aa_key;
typedef struct aa_node *aa_tree;

extern void *aa_find (aa_tree, aa_key);
extern void *aa_insert (aa_tree *, aa_key, size_t);
extern void aa_delete (aa_tree *, aa_key);
extern void aa_free (aa_tree *);
extern void aa_traverse (aa_tree, void (*)(aa_key, void *, void *), void *);
