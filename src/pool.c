/*
 * Copyright (c) 2010-2015, Arnaud TROEL
 * See LICENSE file for license details.
 */

#include "b6/pool.h"

static void initialize_chunk(struct b6_pool *pool, struct b6_chunk *chunk)
{
	int dir;
	struct b6_tref *top, *ref;

	chunk->index = sizeof(struct b6_chunk);
	chunk->free = pool->chunk_size - chunk->index;
	chunk->used = 0;
	chunk->flag = 0;

	b6_list_add_first(&pool->list, &chunk->dref);

	b6_tree_search(&pool->tree, ref, top, dir)
		dir = ref < &chunk->tref ? B6_NEXT : B6_PREV;
	b6_tree_add(&pool->tree, top, dir, &chunk->tref);
}

static void finalize_chunk(struct b6_pool *pool, struct b6_chunk *chunk)
{
	struct b6_tref *top, *ref;
	int dir;

	if (chunk == pool->curr)
		pool->curr = NULL;

	b6_list_del(&chunk->dref);

	b6_tree_search(&pool->tree, ref, top, dir) {
		char *ptr = (char *)b6_cast_of(ref, struct b6_chunk, tref);
		if (ptr < (char *)chunk)
			dir = B6_NEXT;
		else if (ptr >= (char *)chunk + pool->chunk_size)
			dir = B6_PREV;
		else
			break;
	}
	b6_tree_del(&pool->tree, top, dir);
}

static struct b6_chunk *allocate_chunk(struct b6_pool *pool)
{
	struct b6_chunk *chunk = pool->free;

	if (chunk == NULL)
		return b6_allocate(pool->allocator, pool->chunk_size);

	pool->free = NULL;

	return chunk;
}

static void release_chunk(struct b6_pool *pool, struct b6_chunk *chunk)
{
	if (pool->free == NULL)
		pool->free = chunk;
	else
		b6_deallocate(pool->allocator, chunk);
}

static struct b6_chunk *find_chunk(struct b6_pool *pool, void *ptr)
{
	struct b6_tref *ref, *top;
	int dir;

	b6_tree_search(&pool->tree, ref, top, dir) {
		struct b6_chunk *chunk = b6_cast_of(ref, struct b6_chunk, tref);
		if ((char *)chunk > (char *)ptr)
			dir = B6_PREV;
		else if ((char *)chunk + pool->chunk_size < (char *)ptr)
			dir = B6_NEXT;
		else
			return chunk;
	}

	return NULL;
}

void *b6_pool_get(struct b6_pool *pool)
{
	struct b6_chunk *chunk;
	void *ptr;

	/* First check whether something is available. */
	while (!b6_deque_empty(&pool->queue)) {
		/* Get the next free item and find the chunk it belongs to. */
		struct b6_sref *sref;

		sref = b6_deque_del_first(&pool->queue);
		chunk = find_chunk(pool, sref);

		b6_assert(chunk);

		if (!chunk->flag) {
			chunk->used += 1;
			return sref;
		}

		/* The object cannot be used as it belongs to a chunk to
		   delete. We take the opportunity to update the chunk
		   status. */
		chunk->free += pool->size;
		if (chunk->free == pool->chunk_size - sizeof(*chunk)) {
			finalize_chunk(pool, chunk);
			release_chunk(pool, chunk);
		}
	}

	/* Nothing found: Bummer! We have to allocate a new object. */
	if (!pool->curr || pool->curr->index + pool->size > pool->chunk_size) {
		/* A new chunk must be allocated. */
		chunk = allocate_chunk(pool);

		if (!chunk)
			return NULL;

		initialize_chunk(pool, chunk);
		pool->curr = chunk;
	}

	/* Allocate a ptr within the chunk. */
	ptr = ((char *)pool->curr) + pool->curr->index;
	pool->curr->used += 1;
	pool->curr->free -= pool->size;
	pool->curr->index += pool->size;

	return ptr;
}

void b6_pool_put(struct b6_pool *pool, void *ptr)
{
	struct b6_chunk *chunk;
	struct b6_sref *sref;

	chunk = find_chunk(pool, ptr);
	sref = (struct b6_sref *)ptr;

	b6_deque_add_first(&pool->queue, sref);
	chunk->used -= 1;
	chunk->flag = (chunk->used == 0);
}

void b6_pool_finalize(struct b6_pool *pool)
{
	/* free allocated chunks. */
	while (!b6_list_empty(&pool->list)) {
		struct b6_dref *ref = b6_list_first(&pool->list);
		struct b6_chunk *chunk = b6_cast_of(ref, struct b6_chunk, dref);
		finalize_chunk(pool, chunk);
		release_chunk(pool, chunk);
	}

	/* free possibly cached free chunk. */
	if (pool->free) release_chunk(pool, pool->free);
}

static void *b6_pool_allocate(struct b6_allocator *self, unsigned long size)
{
	struct b6_pool *pool = b6_cast_of(self, struct b6_pool, parent);
	return size > pool->size ? NULL : b6_pool_get(pool);
}

static void *b6_pool_reallocate(struct b6_allocator *self, void *ptr,
				unsigned long size)
{
	struct b6_pool *pool = b6_cast_of(self, struct b6_pool, parent);
	return size > pool->size ? NULL : ptr;
}

static void b6_pool_deallocate(struct b6_allocator *self, void *ptr)
{
	struct b6_pool *pool = b6_cast_of(self, struct b6_pool, parent);
	b6_pool_put(pool, ptr);
}

int b6_pool_initialize(struct b6_pool *pool, struct b6_allocator *allocator,
		       unsigned size, unsigned chunk_size)
{
	static const struct b6_allocator_ops ops = {
		.allocate = b6_pool_allocate,
		.reallocate = b6_pool_reallocate,
		.deallocate = b6_pool_deallocate,
	};

	if (!allocator) return -1;
	if (!size) return -1;

	/* align the size of a ptr to a multiple of queue_node */
	b6_static_assert(sizeof(struct b6_sref) == sizeof(void*));
	b6_static_assert(__b6_is_apot(sizeof(void*)));
	size = (size + (sizeof(void*) - 1)) & ~(sizeof(void*) - 1);
	b6_assert(!(size % sizeof(struct b6_sref)));

	/* calculate and/or check chunk_size */
	b6_static_assert(sizeof(struct b6_chunk) < 4096 - sizeof(void*));
	if (!chunk_size) {
		for (chunk_size = 4096; chunk_size - sizeof(struct b6_chunk) -
		     sizeof(void*) < size; chunk_size *= 2)
			if (!chunk_size)
				return -1;
		chunk_size -= sizeof(void*);
	} else if (chunk_size < sizeof(struct b6_chunk) ||
		   chunk_size - sizeof(struct b6_chunk) < size)
		return -1;

	pool->parent.ops = &ops;
	pool->chunk_size = chunk_size;
	pool->size = size;
	pool->curr = NULL;
	pool->free = NULL;
	pool->allocator = allocator;
	b6_deque_initialize(&pool->queue);
	b6_list_initialize(&pool->list);
	b6_tree_initialize(&pool->tree, &b6_tree_avl_ops);

	return 0;
}
