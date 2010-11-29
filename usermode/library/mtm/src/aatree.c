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

#include "mtm_i.h"


typedef unsigned int aa_level;

typedef struct aa_node
{
  struct aa_node *link[2];
  aa_key key;
  aa_level level;
  char data[] __attribute__((aligned));
} aa_node;

#define L		0
#define R		1
#define NIL		((aa_tree)&aa_nil)


/* The code for rebalancing the tree is greatly simplified by never
   having to check for null pointers.  Instead, leaf node links point
   to this node, NIL, which points to itself.  */
static const aa_node aa_nil = { { NIL, NIL }, 0, 0 };

/* Remove left horizontal links.  Swap the pointers of
   horizontal left links.  */

static aa_tree 
skew (aa_tree t)
{
  aa_tree l = t->link[L];
  if (t->level != 0 && l->level == t->level)
    {
      t->link[L] = l->link[R];
      l->link[R] = t;
      return l;
    }
  return t;
}

/* Remove consecutive horizontal links.  Take the middle node,
   elevate it, and return it.  */

static aa_tree 
split (aa_tree t)
{
  aa_tree r = t->link[R];
  if (t->level != 0 && r->link[R]->level == t->level)
    {
      t->link[R] = r->link[L];
      r->link[L] = t;
      r->level += 1;
      return r;
    }
  return t;
}

/* Decrease the level of T to be one more than the level of its children.  */

static void
decrease_level (aa_tree t)
{
  aa_tree l = t->link[L];
  aa_tree r = t->link[R];
  aa_level llev = l->level;
  aa_level rlev = r->level;
  aa_level should_be = (llev < rlev ? llev : rlev) + 1;

  if (should_be < t->level)
    {
      t->level = should_be;
      if (should_be < rlev)
	r->level = should_be;
    }
}

/* Allocate a new node for KEY, with extra memory SIZE.  */

static aa_tree 
aa_new (uintptr_t key, size_t size)
{
  aa_tree n = malloc (sizeof (*n) + size);

  n->link[0] = NIL;
  n->link[1] = NIL;
  n->key = key;
  n->level = 1;

  return n;
}

/* Find the node within T that has KEY.  If found, return the data
   associated with the key.  */

void *
aa_find (aa_tree t, aa_key key)
{
  if (t != NULL)
    while (t != NIL)
      {
	if (t->key == key)
	  return t->data;
	t = t->link[key > t->key];
      }

  return NULL;
}

/* Insert N into T and rebalance.  Return the new balanced tree.  */

static aa_tree 
aa_insert_1 (aa_tree t, aa_tree n)
{
  int dir = n->key > t->key;
  aa_tree c = t->link[dir];

  /* Insert the node, recursively.  */
  if (c == NIL)
    c = n;
  else
    c = aa_insert_1 (c, n);
  t->link[dir] = c;

  /* Rebalance the tree, as needed.  */
  t = skew (t);
  t = split (t);

  return t;
}

/* Insert a new node with KEY into the tree rooted at *PTREE.  Create
   the new node with extra memory SIZE.  Return a pointer to the extra
   memory associated with the new node.  It is invalid to insert a 
   duplicate key.  */

void *
aa_insert (aa_tree *ptree, aa_key key, size_t size)
{
  aa_tree n = aa_new (key, size);
  aa_tree t = *ptree;

  if (t == NULL)
    t = n;
  else
    t = aa_insert_1 (t, n);
  *ptree = t;

  return n->data;
}

/* Delete KEY from T and rebalance.  Return the new balanced tree.  */

static aa_tree 
aa_delete_1 (aa_tree t, aa_key key, bool do_free)
{
  aa_tree r;
  int dir;

  /* If this is the node we're looking for, delete it.  Else recurse.  */
  if (key == t->key)
    {
      aa_tree l, sub, end;

      l = t->link[L];
      r = t->link[R];

      if (do_free)
	free (t);

      /* If this is a leaf node, simply remove the node.  Otherwise,
	 we have to find either a predecessor or a successor node to
	 replace this one.  */
      if (l == NIL)
	{
	  if (r == NIL)
	    return NIL;
	  sub = r, dir = L;
	}
      else
	sub = l, dir = R;

      /* Find the successor or predecessor.  */
      for (end = sub; end->link[dir] != NIL; end = end->link[dir])
	continue;

      /* Remove it (but don't free) from the subtree.  */
      sub = aa_delete_1 (sub, end->key, false);

      /* Replace T with the successor we just extracted.  */
      end->link[1-dir] = sub;
      t = end;
    }
  else
    {
      dir = key > t->key;
      t->link[dir] = aa_delete_1 (t->link[dir], key, do_free);
    }

  /* Rebalance the tree.  */
  decrease_level (t);
  t = skew (t);
  t->link[R] = r = skew (t->link[R]);
  r->link[R] = skew (r->link[R]);
  t = split (t);
  t->link[R] = split (t->link[R]);

  return t;
}

/* Delete KEY from the tree rooted at *PTREE.  */

void
aa_delete (aa_tree *ptree, aa_key key)
{
  aa_tree t = *ptree;

  if (t == NULL)
    return;

  t = aa_delete_1 (t, key, true);
  if (t == NIL)
    t = NULL;
  *ptree = t;
}

/* Free the tree at T.  */

static void
aa_free_1 (aa_tree t)
{
  int dir;
  for (dir = 0; dir < 2; ++dir)
    {
      aa_tree c = t->link[dir];
      if (c != NIL)
	aa_free_1 (c);
    }
  free (t);
}

/* Free the tree rooted at *PTREE.  */

void
aa_free (aa_tree *ptree)
{
  aa_tree t = *ptree;
  if (t != NULL)
    aa_free_1 (t);
  *ptree = NULL;
}

/* Invoke CALLBACK on each node of T.  */

static void
aa_traverse_1 (aa_tree t, void (*callback)(aa_key, void *, void *),
	       void *callback_data)
{
  if (t == NIL)
    return;

  callback (t->key, t->data, callback_data);

  aa_traverse (t->link[L], callback, callback_data);
  aa_traverse (t->link[R], callback, callback_data);
}

void
aa_traverse (aa_tree t, void (*callback)(aa_key, void *, void *),
	     void *callback_data)
{
  if (t != NULL)
    aa_traverse_1 (t, callback, callback_data);
}
