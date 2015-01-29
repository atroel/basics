/*
 * Copyright (c) 2010-2015, Arnaud TROEL
 * See LICENSE file for license details.
 */

#include "b6/array.h"
#include "b6/allocator.h"

static int b6_array_resize(struct b6_array *self, unsigned long int capacity)
{
	void *buffer;
	unsigned long long int size;
	size = (unsigned long long int)self->itemsize * capacity;
	if (size > ~0UL)
		return -2;
	buffer = b6_reallocate(self->allocator, self->buffer, size);
	if (!buffer)
		return -1;
	self->buffer = buffer;
	self->capacity = capacity;
	return 0;
}

int b6_array_expand(struct b6_array *self, unsigned long int n)
{
	unsigned long int capacity;
	for (capacity = self->capacity ? self->capacity * 2 : 2;
	     n >= capacity - self->length; capacity *= 2)
		if (capacity < self->capacity)
			return 0;
	return b6_array_resize(self, capacity);
}

int b6_array_shrink(struct b6_array *self)
{
	unsigned long int capacity;
	if (!self->length) {
		b6_deallocate(self->allocator, self->buffer);
		self->buffer = NULL;
		self->capacity = 0UL;
		return 0;
	}
	for (capacity = self->capacity; capacity / 2 > self->length;
	     capacity /= 2);
	return b6_array_resize(self, capacity);
}
