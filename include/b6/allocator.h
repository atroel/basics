/*
 * Copyright (c) 2010, Arnaud TROEL
 * See LICENSE file for license details.
 */

#ifndef B6_ALLOCATOR_H_
#define B6_ALLOCATOR_H_

#include "b6/assert.h"
#include "b6/utils.h"

struct b6_allocator {
	const struct b6_allocator_ops *ops;
};

struct b6_allocator_ops {
	void *(*allocate)(struct b6_allocator*, unsigned long int);
	void *(*reallocate)(struct b6_allocator*, void*, unsigned long int);
	void (*deallocate)(struct b6_allocator*, void*);
};

extern struct b6_allocator b6_oom_allocator;

static inline void *b6_allocate(struct b6_allocator *self,
				unsigned long int size)
{
	b6_precond(self->ops);
	b6_precond(self->ops->allocate);
	return self->ops->allocate(self, size);
}

static inline void *b6_reallocate(struct b6_allocator *self, void *ptr,
				  unsigned long int size)
{
	b6_precond(self->ops);
	if (!ptr)
		return b6_allocate(self, size);
	if (self->ops->reallocate)
		return self->ops->reallocate(self, ptr, size);
	return NULL;
}

static inline void b6_deallocate(struct b6_allocator *self, void *ptr)
{
	b6_precond(self->ops);
	b6_precond(self->ops->deallocate);
	if (ptr)
		self->ops->deallocate(self, ptr);
}

struct b6_fixed_allocator {
	struct b6_allocator allocator;
	void *buf;
	unsigned long int len;
	unsigned long int pos;
};

extern const struct b6_allocator_ops b6_fixed_allocator_ops;

static inline void b6_reset_fixed_allocator(struct b6_fixed_allocator *self,
					    void *buf, unsigned long int len)
{
	self->allocator.ops = &b6_fixed_allocator_ops;
	self->buf = buf;
	self->len = len;
	self->pos = 0;
}

#endif /* B6_ALLOCATOR_H_ */
