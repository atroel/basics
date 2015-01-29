/*
 * Copyright (c) 2009-2015, Arnaud TROEL
 * See LICENSE file for license details.
 */

#ifndef B6_REFS_H_
#define B6_REFS_H_

#include "assert.h"

/**
 * @file refs.h
 *
 * @brief Containers references
 *
 * Containers are objects that allows building collections of homogeneous
 * data, so-called elements. They are of two main types. Sequential containers
 * stores elements in a raw. Associative containers stores elements in an
 * ordered manner.
 *
 * At glance, sequential containers are fast for adding and removing elements
 * but behave poorly when being searched. Associative containers require
 * elements to be comparable. They are slower for insertion and removal but
 * provide efficient search algorithms.
 *
 * Containers use references to keep track of elements. A reference is a data
 * structure that holds one or several pointer to other references.
 *
 * Elements contains a reference as a member. It is possible to dereference a
 * reference to get the pointer of the element it is bound to using
 * b6_cast_of. Here is an example with a simply-linked reference:
 *
 * @code
 * struct element {
 * 	...
 * 	struct b6_sref sref;
 * 	...
 * };
 *
 * struct element *foo(struct b6_sref *ref)
 * {
 *  	return b6_cast_of(ref, struct element, sref);
 * }
 * @endcode
 *
 * Note that not every reference is linked to an element: every container has
 * head and tail references that are placed before and after any reference
 * within the container respectively. It is illegal to dereference them.
 *
 * @see b6_sref, b6_dref, b6_tref
 * @see deque.h, list.h, vector.h, splay.h, tree.h
 */

enum { B6_NEXT, B6_PREV };

/**
 * @brief Get the direction from a comparison result
 * @param weight -1 or 1 according to previous comparison
 * @return B6_PREV if weight equal -1 or B6_NEXT if weight equals 1
 */
static inline int b6_to_direction(int weight)
{
	int dir;
	b6_precond(weight == -1 || weight == 1);
	dir = (weight >> 1) & 1;
	b6_precond((weight == 1 && dir == B6_NEXT) ||
		   (weight == -1 && dir == B6_PREV));
	return dir;
}

/**
 * @brief Get the opposite direction
 * @param dir direction to get the opposite from
 * @return the opposite
 */
static inline int b6_to_opposite(int dir)
{
	int opp;
	b6_precond(dir == B6_NEXT || dir == B6_PREV);
	opp = dir ^ 1;
	b6_postcond((dir == B6_NEXT && opp == B6_PREV) ||
		    (opp == B6_NEXT && dir == B6_PREV));
	return opp;
}

/**
 * @brief Single reference
 * @see deque.h
 */
struct b6_sref {
	struct b6_sref *ref; /**< pointer to the next reference */
};

/**
 * @brief Double reference
 * @see list.h, splay.h
 */
struct b6_dref {
	struct b6_dref *ref[2]; /**< pointers to other references */
};

/**
 * @brief Triple reference
 * @see tree.h
 */
struct b6_tref {
	struct b6_tref *ref[2]; /**< pointers to other references */
	struct b6_tref *top; /**< pointer to parent reference */
};

/**
 * @brief function for comparing two elements
 * @param l pointer to the left element
 * @param r pointer to the right element
 * @retval 0 if l and r are equal
 * @retval -1 if l is strictly greater than r
 * @retval 1 if r is strictly greater than l
 */
typedef int (*b6_compare_t)(void *l, void *r);

#endif /* B6_REFS_H_ */
