/*
 * Copyright (c) 2009-2015, Arnaud TROEL
 * See LICENSE file for license details.
 */

/**
 * @file pool.h
 * @brief This file implements a cached memory allocator.
 * @author Arnaud TROEL
 */

#ifndef POOL_H_
#define POOL_H_

#include "tree.h"
#include "list.h"
#include "deque.h"
#include "allocator.h"

/**
 * A memory allocator using cached chunks where it actually allocates objects
 * of constant size.
 */
struct b6_pool {
	struct b6_allocator parent; /**< a pool is a kind of allocator */

	unsigned chunk_size; /**< Size in bytes of a chunk. */
	unsigned size; /**< Size in bytes of an object. */

	struct b6_chunk *curr; /**< Chunk within objects are allocated. */
	struct b6_chunk *free; /**< Cached free chunk for fast allocation. */

	struct b6_deque queue; /**< Queue of free objects. */
	struct b6_list list; /**< List of chunks. */
	struct b6_tree tree; /**< Tree of chunks. */

	struct b6_allocator *allocator; /**< Underlying allocator. */
};

/** The chunk of the pool which objects are allocated in. */
struct b6_chunk {
	struct b6_tref tref; /**< Chunks are organized in a tree. */
	struct b6_dref dref; /**< Chunks are organized in a list. */
	unsigned int free; /**< Free bytes remaining in the chunk. */
	unsigned int used; /**< Allocated objects in the chunk. */
	unsigned int index; /**< Offset of the 1st free object in the chunk. */
	unsigned int flag; /**< Whether this chunk is to be deleted. */
};

/**
 * Initialize a pool allocator.
 * @param pool specifies the pool to initialize.
 * @param size specifies the size of object this allocator will produce.
 * @param chunk_size specifies the size of memory chunk to allocate.
 * @param allocator specifies the allocator used for dynamically allocating
 *        chunks.
 */
int b6_pool_initialize(struct b6_pool *pool, struct b6_allocator *allocator,
		       unsigned size, unsigned chunk_size);

/**
 * Finalize a pool allocator.
 * @param pool specifies the pool to finalize.
 *
 * This operation releases the whole memory that has been allocated. Any
 * previously allocated object is automatically released and cannot be used
 * anymore. The pool cannot be used anymore until it is initialized once again.
 */
void b6_pool_finalize(struct b6_pool *pool);

/**
 * Allocate an object.
 * @param pool specifies the pool within the object has to be allocated.
 * @return a pointer to the object or NULL if allocation failed.
 */
void *b6_pool_get(struct b6_pool *pool);

/**
 * Release an object.
 * @param pool specifies the pool within the object was allocated.
 * @param ptr specifies the object to release.
 */
void b6_pool_put(struct b6_pool *pool, void *ptr);

#endif /* POOL_H_ */
