/*
 * Copyright (c) 2014, Arnaud TROEL
 * See LICENSE file for license details.
 */

/**
 * @file heap.h
 * @brief Binomial heap or priority queue.
 */

#ifndef B6_HEAP_H
#define B6_HEAP_H

#include "b6/array.h"
#include "b6/assert.h"
#include "b6/refs.h"
#include "b6/utils.h"

/**
 * @brief A heap is a container of comparable items such that the most
 * prioritary one (with regard to how items compare) can be immediately
 * acccessed.
 *
 * This implementation uses an underlying array of pointers to items. It should
 * not be mutated while the heap API is used.
 */
struct b6_heap {
	struct b6_array *array; /**< underlying array */
	b6_compare_t compare; /**< items comparator */
	void (*set_index)(void*, unsigned long int); /**< item index callback */
};


/**
 * @internal
 */
extern void b6_heap_do_make(struct b6_heap*);

/**
 * @internal
 */
extern void b6_heap_do_pop(struct b6_heap*);

/**
 * @internal
 */
extern void b6_heap_do_push(struct b6_heap*, void**, unsigned long int);

/**
 * @internal
 */
extern void b6_heap_do_boost(struct b6_heap*, void**, unsigned long int);

/**
 * @brief Make a heap out of an array.
 *
 * This function must be called first or results of other functions are
 * unpredicted.
 *
 * @complexity O(n)
 * @param self specifies the heap to initialize.
 * @param array specifies the underlying array of elements pointers.
 * @param compare specifies the function to call back to compare to items so as
 * to get the most prioritary one.
 * @param set_index specifies an optional function to call back when an item is
 * assigned an index in the underlying array.
 */
static inline void b6_heap_reset(struct b6_heap *self,
				 struct b6_array *array,
				 b6_compare_t compare,
				 void (*set_index)(void*, unsigned long int))
{
	b6_assert(array->itemsize == sizeof(void*));
	self->array = array;
	self->compare = compare;
	self->set_index = set_index;
	b6_heap_do_make(self);
}

/**
 * @brief Return how many items a heap contains.
 * @complexity O(1)
 * @param self specifies the heap.
 * @return how many items the heap contains.
 */
static inline unsigned long int b6_heap_length(const struct b6_heap *self)
{
	return b6_array_length(self->array);
}

/**
 * @brief Return if a heap contains any items.
 * @complexity O(1)
 * @param self specifies the heap.
 * @return true if the heap is empty.
 */
static inline int b6_heap_empty(const struct b6_heap *self)
{
	return !b6_heap_length(self);
}

/**
 * @brief Get access to the item on the top of the heap.
 * @pre The heap must not be empty.
 * @complexity O(1)
 * @param self specifies the heap.
 * @return A pointer to the top item.
 */
static inline void *b6_heap_top(const struct b6_heap *self)
{
	void **ptr = b6_array_get(self->array, 0);
	b6_assert(!b6_heap_empty(self));
	return *ptr;
}

/**
 * @brief Remove the item on the top of the heap.
 * @pre The heap must not be empty.
 * @complexity O(log(n))
 * @param self specifies the heap.
 */
static inline void b6_heap_pop(struct b6_heap *self)
{
	b6_assert(!b6_heap_empty(self));
	b6_heap_do_pop(self);
	b6_array_reduce(self->array, 1);
}

/**
 * @brief Insert a new item in the heap.
 * @complexity O(log(n))
 * @param self specifies the heap.
 * @param item specifies the item to insert.
 * @return 0 for success
 * @return -1 when out of memory
 */
static inline int b6_heap_push(struct b6_heap *self, void *item)
{
	unsigned long int len = b6_array_length(self->array);
	void **ptr = b6_array_extend(self->array, 1);
	if (!ptr)
		return -1;
	*ptr = item;
	if (self->set_index)
		self->set_index(item, len);
	b6_heap_do_push(self, b6_array_get(self->array, 0), len);
	return 0;
}

/**
 * @brief Move an item towards the top of the heap.
 *
 * This function can be called when the priority of an item has increased so
 * that it can be moved in the heap properly.
 *
 * @see b6_heap_extract if the priority of the item has decreased.
 * @see b6_heap_initialize to get notified of the index of items.
 *
 * @complexity O(log(n))
 * @param self specifies the heap.
 * @param index specifies the index of the item to promote.
 */
static inline void b6_heap_touch(struct b6_heap *self, unsigned long int index)
{
	void **buf = b6_array_get(self->array, 0);
	b6_assert(index < b6_array_length(self->array));
	b6_heap_do_push(self, buf, index);
}

/**
 * @brief Removes an item from the heap.
 *
 * This function can be called to remove a specific item from the heap. Another
 * use case is when the priority of an item has decreased. This function allows
 * to remove it so as to re-insert it afterwards.
 *
 * @see b6_heap_initialize to get notified of the index of items.
 *
 * @complexity O(log(n))
 * @param self specifies the heap.
 * @param index specifies the index of the item to remove.
 */
static inline void b6_heap_extract(struct b6_heap *self,
				   unsigned long int index)
{
	void **buf = b6_array_get(self->array, 0);
	b6_assert(index < b6_array_length(self->array));
	b6_heap_do_boost(self, buf, index);
	b6_heap_pop(self);
}

#endif /* B6_HEAP_H */
