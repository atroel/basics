/*
 * Copyright (c) 2010-2015, Arnaud TROEL
 * See LICENSE file for license details.
 */

/**
 * @file array.h
 * @brief Arrays of items.
 */

#ifndef B6_ARRAY_H_
#define B6_ARRAY_H_

#include "assert.h"
#include "allocator.h"

/**
 * @brief An array is a sequence of items which can be accessed randomly.
 *
 * An array can be extended or reduced from its end only.
 * Arrays rely on an allocator which is automatically called to increase or
 * decrease their size in memory.
 */
struct b6_array {
	struct b6_allocator *allocator; /**< underlying memory allocator */
	unsigned long int itemsize; /**< size in bytes of an item */
	unsigned long int capacity; /**< number of items before allocation */
	unsigned long int length; /**< number of items in the array */
	unsigned char *buffer; /**< pointer to the items */
};

/**
 * @brief Initialize an array.
 *
 * This function must be called before an array can be used.
 *
 * @param self specifies the array to initialize.
 * @param allocator specifies the memory allocator to use for the array.
 * @param itemsize specifies the size in bytes of items in the array.
 */
static inline void b6_array_initialize(struct b6_array *self,
				       struct b6_allocator *allocator,
				       unsigned long int itemsize)
{
	b6_precond(self);
	b6_precond(allocator);
	b6_precond(itemsize);
	self->allocator = allocator;
	self->itemsize = itemsize;
	self->capacity = 0;
	self->length = 0;
	self->buffer = NULL;
}

/**
 * @brief Remove all items from an array and release its resources.
 *
 * Once this function has been called, the array cannot be used anymore until it
 * is initialized again.
 *
 * @param self specifies the array to finalize.
 */
static inline void b6_array_finalize(struct b6_array *self)
{
	b6_deallocate(self->allocator, self->buffer);
}

/**
 * @brief How many items can be currently contained by the array.
 * @param self specifies the array.
 * @return The maximum number of items the array can contain until further
 * memory allocation is needed.
 */
static inline unsigned long int b6_array_capacity(const struct b6_array *self)
{
	b6_precond(self);
	return self->capacity;
}

/**
 * @brief Number of items the array actually contains.
 * @param self specifies the array.
 * @return The number of items the array contains.
 */
static inline unsigned long int b6_array_length(const struct b6_array *self)
{
	b6_precond(self);
	return self->length;
}

/**
 * @brief Erase the whole array.
 * @param self specifies the array to clear.
 */
static inline void b6_array_clear(struct b6_array *self)
{
	b6_precond(self);
	self->length = 0;
}

/**
 * @brief Size of the whole array in memory.
 * @param self specifies the array.
 * @return The size in bytes of the array.
 */
static inline unsigned long int b6_array_memsize(const struct b6_array *self)
{
	return self->itemsize * self->capacity;
}

/**
 * @brief Exchange the contents of two arrays.
 *
 * The performance of this operation is independent from the actual sizes of the
 * arrays.
 *
 * @param lhs specifies one array (left hand side).
 * @param rhs specifies the other array (right hand side).
 */
static inline void b6_array_swap(struct b6_array *lhs, struct b6_array *rhs)
{
	struct b6_array temp;
	b6_precond(lhs);
	b6_precond(rhs);
	temp.allocator = lhs->allocator;
	temp.itemsize = lhs->itemsize;
	temp.capacity = lhs->capacity;
	temp.length = lhs->length;
	temp.buffer = lhs->buffer;
	lhs->allocator = rhs->allocator;
	lhs->itemsize = rhs->itemsize;
	lhs->capacity = rhs->capacity;
	lhs->length = rhs->length;
	lhs->buffer = rhs->buffer;
	rhs->allocator = temp.allocator;
	rhs->itemsize = temp.itemsize;
	rhs->capacity = temp.capacity;
	rhs->length = temp.length;
	rhs->buffer = temp.buffer;
}

/**
 * @brief Access the contents of an array.
 * @param self specifies the array.
 * @param index specifies which item of the array to access (first is 0).
 * @return NULL if index is out of the bounds of the array.
 * @return A pointer to the items which remains valid until the next extend or
 * reduce operation.
 */
static inline void *b6_array_get(const struct b6_array *self,
				 unsigned long int index)
{
	unsigned long long int offset;
	b6_precond(self);
	if (index >= self->length)
		return NULL;
	offset = (unsigned long long int)self->itemsize * index;
	if (offset > ~0UL)
		return NULL;
	return &self->buffer[offset];
}

/**
 * @internal
 */
extern int b6_array_expand(struct b6_array*, unsigned long int);

/**
 * @internal
 */
extern int b6_array_shrink(struct b6_array*);

/**
 * @brief Append items to the array.
 *
 * Once appended, items are left uninitialized. A pointer is returned to the
 * caller so that they can be set up properly.
 *
 * @param self specifies the array.
 * @param n specifies how many items to append to the array.
 * @return A pointer to the first item added.
 * @return NULL when out of memory.
 */
static inline void *b6_array_extend(struct b6_array *self, unsigned long int n)
{
	void *ptr;
	b6_precond(self);
	if (n > self->capacity - self->length && b6_array_expand(self, n))
		return NULL;
	ptr = self->buffer +
		(unsigned long long int)self->itemsize * self->length;
	self->length += n;
	return ptr;
}

/**
 * @brief Remove trailing items from the array.
 * @param self specifies the array.
 * @param n specifies how many items to remove at the end of the array.
 * @return how many items have been removed.
 */
static inline unsigned long int b6_array_reduce(struct b6_array *self,
						unsigned long int n)
{
	b6_precond(self);
	if (n > self->length)
		n = self->length;
	self->length -= n;
	b6_array_shrink(self);
	return n;
}

#endif /* B6_ARRAY_H_ */
