#include "b6/allocator.h"

static void *b6_oom_allocate(struct b6_allocator *self, unsigned long int size)
{
	return NULL;
}

static const struct b6_allocator_ops b6_oom_allocator_ops = {
	.allocate = b6_oom_allocate,
};

struct b6_allocator b6_oom_allocator = { .ops = &b6_oom_allocator_ops, };

static void *b6_fixed_allocator_allocate(struct b6_allocator *allocator,
					 unsigned long int size)
{
	struct b6_fixed_allocator *self =
		b6_cast_of(allocator, struct b6_fixed_allocator, allocator);
	if (self->pos || size > self->len)
		return 0;
	self->pos = size;
	return self->buf;
}

static void *b6_fixed_allocator_reallocate(struct b6_allocator *allocator,
					   void *ptr, unsigned long int size)
{
	struct b6_fixed_allocator *self =
		b6_cast_of(allocator, struct b6_fixed_allocator, allocator);
	if (size > self->len)
		return NULL;
	return self->buf;
}

static void b6_fixed_allocator_deallocate(struct b6_allocator *allocator,
					  void *ptr)
{
	struct b6_fixed_allocator *self =
		b6_cast_of(allocator, struct b6_fixed_allocator, allocator);
	self->pos = 0;
}

const struct b6_allocator_ops b6_fixed_allocator_ops = {
	.allocate = b6_fixed_allocator_allocate,
	.reallocate = b6_fixed_allocator_reallocate,
	.deallocate = b6_fixed_allocator_deallocate,
};
