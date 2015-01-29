/*
 * Copyright (c) 2010-2015, Arnaud TROEL
 * See LICENSE file for license details.
 */

#ifndef B6_LIST_H_
#define B6_LIST_H_

#include "refs.h"
#include "utils.h"
#include "assert.h"

/**
 * @defgroup list Doubly-linked list
 *
 * Doubly-linked list is a deque where each element is linked to its successor
 * and its predecessor in the sequence. As a result, there are no "after"
 * operations anymore as inserting or removing an element before a reference
 * offer the same performance.
 *
 * Fast operations on deque are faster than their equivalent on lists, even if
 * they have the same overall complexity. Moreover, lists have a slightly
 * bigger memeory footprint.
 */

/**
 * @brief Doubly-linked list
 * @ingroup list
 * @see b6_dref
 */
struct b6_list {
	struct b6_dref dref; /**< sentinel of the list */
};

#define B6_LIST_INIT(list) { { { &(list).dref, &(list).dref } } }

/**
 * @brief Initialize a list statically
 * @ingroup list
 * @param list name of the variable
 */
#define B6_LIST_DEFINE(list) struct b6_list list = B6_LIST_INIT(list)

/**
 * @brief Initialize or clear a doubly-linked list
 * @ingroup list
 * @complexity O(1)
 * @param list pointer to the doubly-linked list
 */
static inline void b6_list_initialize(struct b6_list *list)
{
	b6_precond(list);

	list->dref.ref[B6_NEXT] = &list->dref;
	list->dref.ref[B6_PREV] = &list->dref;
}

/**
 * @ingroup list
 * @brief Return the reference of the head of a doubly-linked list
 * @complexity O(1)
 * @param list pointer to the doubly-linked list
 * @return pointer to a reference which next reference is the first element in
 * the doubly-linked list
 *
 * The head reference cannot be dereferenced as it is associated with no
 * element.
 */
static inline struct b6_dref *b6_list_head(const struct b6_list *list)
{
	b6_precond(list);

	return (struct b6_dref *)&list->dref;
}

/**
 * @ingroup list
 * @brief Return the reference of the tail of a doubly-linked list
 * @complexity O(1)
 * @param list pointer to the doubly-linked list
 * @return pointer to a reference which previous reference is the last element
 * in the doubly-linked list
 *
 * The tail reference cannot be dereferenced as it is associated with no
 * element.
 */
static inline struct b6_dref *b6_list_tail(const struct b6_list *list)
{
	b6_precond(list);

	return (struct b6_dref *)&list->dref;
}

/**
 * @brief Travel a doubly-linked list reference per reference
 * @ingroup list
 * @complexity O(1)
 * @param dref reference to walk from
 * @param direction B6_PREV or B6_NEXT to get the previous or next reference
 * respectively
 * @return NULL when going out of range or the next or previous reference in
 * the sequence
 */
static inline struct b6_dref *b6_list_walk(const struct b6_dref *dref,
                                           int direction)
{
	b6_precond((unsigned)direction < b6_card_of(dref->ref));
	b6_precond(dref);

	return (struct b6_dref *)dref->ref[direction];
}

/**
 * @brief Return the reference of the first element of a doubly-linked list
 * @ingroup list
 * @complexity O(1)
 * @param list pointer to the doubly-linked list
 * @return pointer to the reference of the first element or tail if the
 * list is empty
 */
static inline struct b6_dref *b6_list_first(const struct b6_list *list)
{
	return b6_list_walk(b6_list_head(list), B6_NEXT);
}

/**
 * @brief Return the reference of the last element of a doubly-linked list
 * @ingroup list
 * @complexity O(1)
 * @param list pointer to the doubly-linked list
 * @return pointer to the reference of the last element or head if the
 * list is empty
 */
static inline struct b6_dref *b6_list_last(const struct b6_list *list)
{
	return b6_list_walk(b6_list_tail(list), B6_PREV);
}

/**
 * @brief Test if a doubly-linked list contains elements
 * @ingroup list
 * @complexity O(1)
 * @param list pointer to the doubly-linked list
 * @return 0 if the list contains one element or more and another value if it
 * does not contains any elements
 */
static inline int b6_list_empty(const struct b6_list *list)
{
	return b6_list_first(list) == b6_list_tail(list);
}

/**
 * @brief Insert a new element before a reference in the doubly-linked list
 * @ingroup list
 * @complexity O(1)
 * @param next reference in the list
 * @param dref reference of the element to insert
 * @return dref
 */
static inline struct b6_dref *b6_list_add(struct b6_dref *next,
                                          struct b6_dref *dref)
{
	struct b6_dref *prev = b6_list_walk(next, B6_PREV);

	b6_precond(prev);
	prev->ref[B6_NEXT] = dref;
	next->ref[B6_PREV] = dref;

	b6_precond(dref);
	dref->ref[B6_PREV] = prev;
	dref->ref[B6_NEXT] = next;

	return dref;
}

/**
 * @ingroup list
 * @brief Remove an element within the doubly-linked list
 *
 * It is illegal to attempt to remove the head or the tail reference of the
 * doubly-linked list.
 *
 * @complexity O(1)
 * @param dref pointer to the reference in the list
 * @return dref
 */
static inline struct b6_dref *b6_list_del(struct b6_dref *dref)
{
	struct b6_dref *prev = b6_list_walk(dref, B6_PREV);
	struct b6_dref *next = b6_list_walk(dref, B6_NEXT);

	b6_precond(dref != prev);
	b6_precond(dref != next);

	prev->ref[B6_NEXT] = next;
	next->ref[B6_PREV] = prev;

	return dref;
}

/**
 * @brief Insert an element as first element of the doubly-linked list
 * @ingroup list
 * @complexity O(1)
 * @param list pointer to the doubly-linked list
 * @param dref pointer to the reference of the element to insert
 * @return dref
 */
static inline struct b6_dref *b6_list_add_first(struct b6_list *list,
                                                struct b6_dref *dref)
{
	return b6_list_add(b6_list_first(list), dref);
}

/**
 * @brief Insert an element as last element of the doubly-linked list
 * @ingroup list
 * @complexity O(1)
 * @param list pointer to the doubly-linked list
 * @param dref pointer to the reference of the element to insert
 * @return dref
 */
static inline struct b6_dref *b6_list_add_last(struct b6_list *list,
                                               struct b6_dref *dref)
{
	return b6_list_add(b6_list_tail(list), dref);
}

/**
 * @brief Remove the first element of a doubly-linked list
 * @complexity O(1)
 * @pre The doubly-linked list is not empty
 * @param list pointer to the list
 * @return pointer to the reference of the element removed
 */
static inline struct b6_dref *b6_list_del_first(struct b6_list *list)
{
	return b6_list_del(b6_list_first(list));
}

/**
 * @brief Remove the last element of a doubly-linked list
 * @ingroup list
 * @complexity O(1)
 * @pre The doubly-linked list is not empty
 * @param list pointer to the doubly-linked list
 * @return pointer to the reference of the element removed
 */
static inline struct b6_dref *b6_list_del_last(struct b6_list *list)
{
	return b6_list_del(b6_list_last(list));
}

/**
 * @brief Delete elements from a list and add them before another one
 * @ingroup list
 * @complexity O(1)
 * @pre to is after from
 * @pre next is not between from and to
 * @param from first element to move
 * @param to last element to move
 * @param next element before which the sequence will be added
 */
static inline void b6_list_move(struct b6_dref *from, struct b6_dref *to,
				struct b6_dref *next)
{
	struct b6_dref *before = b6_list_walk(from, B6_PREV);
	struct b6_dref *after = b6_list_walk(to, B6_NEXT);
	struct b6_dref *prev = b6_list_walk(next, B6_PREV);

	before->ref[B6_NEXT] = after;
	after->ref[B6_PREV] = before;

	prev->ref[B6_NEXT] = from;
	from->ref[B6_PREV] = prev;

	next->ref[B6_PREV] = to;
	to->ref[B6_NEXT] = next;
}

/**
 * @brief calculate the length of a list
 * @ingroup list
 * @complexity O(n)
 * @param list pointer to the list to calculate the length of
 * @return the length of the list
 */
extern unsigned long int b6_list_length(const struct b6_list *list);

extern void __b6_list_msort(struct b6_list *list, b6_compare_t comp,
			    unsigned long int length);

/**
 * @brief Sort a list using the merge sorting algorithm
 * @complexity O(n log(n))
 * @param list pointer to the list to sort
 * @param comp function to call back for comparing elements of the list
 */
static inline void b6_list_msort(struct b6_list *list, b6_compare_t comp)
{
	unsigned long int length = b6_list_length(list);
	if (length > 1)
		__b6_list_msort(list, comp, length);
}

/**
 * @brief Sort a list using the merge sorting algorithm
 * @complexity O(n log(n))
 * @param list pointer to the list to sort
 * @param comp function to call back for comparing elements of the list
 */
extern void b6_list_qsort(struct b6_list *list, b6_compare_t comp);

#endif /* B6_LIST_H_ */
