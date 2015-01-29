/*
 * Copyright (c) 2010-2015, Arnaud TROEL
 * See LICENSE file for license details.
 */

/**
 * @file tree.h
 *
 * @brief AVL and colored binary search tree container
 */

#ifndef B6_TREE_H_
#define B6_TREE_H_

#include "refs.h"
#include "utils.h"
#include "assert.h"

/**
 * @brief AVL/colored binary search tree data structure
 */
struct b6_tree {
	struct b6_tref tref; /**< sentinel */
	const struct b6_tree_ops *ops;
};

#define B6_TREE_INIT(ops) { { { NULL, NULL }, 0 }, ops }
#define B6_TREE_DEFINE(tree, ops) struct b6_tree tree = B6_TREE_INIT(ops)

struct b6_tree_ops {
	void (*add)(struct b6_tref *top, int dir, struct b6_tref *ref);
	struct b6_tref *(*del)(struct b6_tref *top, int dir);
	int (*chk)(const struct b6_tree *tree, struct b6_tref **tref);
};

extern const struct b6_tree_ops b6_tree_rb_ops;
extern const struct b6_tree_ops b6_tree_avl_ops;

/**
 * @brief Initialize the binary search tree
 * @complexity O(1)
 * @param tree pointer to the tree
 * @param compare pointer the the function to be used for comparing elements
 * @param ops AVL or colored (aka red/black) tree balance policy
 */
static inline void b6_tree_initialize(struct b6_tree *tree,
				      const struct b6_tree_ops *ops)
{
	b6_precond(tree);
	tree->tref.ref[0] = NULL;
	tree->tref.ref[1] = NULL;
	tree->tref.top = NULL;
	tree->ops = ops;
}

/**
 * @brief Return the reference of the head of a tree
 * @complexity O(1)
 * @param tree pointer to the binary search tree
 * @return pointer to a reference which next reference is the first element in
 * the container
 *
 * The head reference cannot be dereferenced as it is associated with no
 * element.
 */
static inline struct b6_tref *b6_tree_head(const struct b6_tree *tree)
{
	b6_precond(tree);
	return (struct b6_tref *)&tree->tref;
}

/**
 * @brief Return the reference of the tail of a tree
 * @complexity O(1)
 * @param tree pointer to the binary search tree
 * @return pointer to a reference which previous reference is the first element
 * in the container
 *
 * The tail reference cannot be dereferenced as it is associated with no
 * element.
 */
static inline struct b6_tref *b6_tree_tail(const struct b6_tree *tree)
{
	b6_precond(tree);
	return (struct b6_tref *)&tree->tref;
}

static inline void b6_tree_top(const struct b6_tree *tree,
			       struct b6_tref **top, int *dir)
{
	b6_precond(tree);
	b6_precond(top);
	b6_precond(dir);
	*top = (struct b6_tref *)&tree->tref;
	*dir = 0;
}

static inline struct b6_tref *b6_tree_child(const struct b6_tref *top, int dir)
{
	b6_precond(top);
	b6_precond((unsigned int)dir < b6_card_of(top->ref));
	return top->ref[dir];
}

static inline struct b6_tref *b6_tree_parent(const struct b6_tref *tref,
					     int *dir)
{
	struct b6_tref *top;
	b6_precond(tref);
	top = (struct b6_tref *)((unsigned long int)tref->top & ~3);
	if (dir)
		*dir = b6_tree_child(top, B6_NEXT) == tref ? B6_NEXT : B6_PREV;
	return top;
}

static inline struct b6_tref *b6_tree_root(const struct b6_tree *tree)
{
	struct b6_tref *top;
	int dir;
	b6_tree_top(tree, &top, &dir);
	return b6_tree_child(top, dir);
}

#define b6_tree_search(tree, ref, top, dir)				\
	b6_precond((tree) != NULL);					\
	for (b6_tree_top(tree, &top, &dir);				\
	     (ref = b6_tree_child(top, dir));				\
	     top = ref)

/**
 * @brief Test if a tree contains elements
 * @complexity O(1)
 * @param tree pointer to the binary search tree
 * @return 0 if the list tree contains one element or more and another value
 * if it does not contains any elements
 */
static inline int b6_tree_empty(const struct b6_tree *tree)
{
	return !b6_tree_root(tree);
}

extern struct b6_tref *__b6_tree_walk(const struct b6_tref *ref, int dir);
extern struct b6_tref *__b6_tree_dive(const struct b6_tref *ref, int dir);

static inline struct b6_tref *b6_tree_walk(const struct b6_tree *tree,
					   const struct b6_tref *tref, int dir)
{
	struct b6_tref *child, *head = b6_tree_head(tree);

	b6_precond(tref);
	b6_precond((unsigned int)dir < b6_card_of(tref->ref));

	if (b6_likely(tref != head))
		if ((child = b6_tree_child(tref, dir)))
			return __b6_tree_dive(child, b6_to_opposite(dir));
		else
			return __b6_tree_walk(tref, dir);
	else if (!b6_tree_empty(tree))
		return __b6_tree_dive(b6_tree_root(tree), b6_to_opposite(dir));
	else
		return head;
}

static inline struct b6_tref *b6_tree_first(const struct b6_tree *tree)
{
	return b6_tree_walk(tree, b6_tree_head(tree), B6_NEXT);
}

static inline struct b6_tref *b6_tree_last(const struct b6_tree *tree)
{
	return b6_tree_walk(tree, b6_tree_tail(tree), B6_PREV);
}

static inline struct b6_tref *b6_tree_add(struct b6_tree *tree,
					  struct b6_tref *top, int dir,
					  struct b6_tref *ref)
{
	b6_precond(top);
	b6_precond((unsigned int)dir < b6_card_of(top->ref));
	b6_precond(!top->ref[dir]);
	b6_precond(ref);
	tree->ops->add(top, dir, ref);
	return ref;
}

static inline struct b6_tref *b6_tree_del(struct b6_tree *tree,
					  struct b6_tref *top, int dir)
{
	b6_precond(top);
	b6_precond((unsigned int)dir < b6_card_of(top->ref));
	b6_precond(top->ref[dir]);
	return tree->ops->del(top, dir);
}

static inline int b6_tree_check(const struct b6_tree *tree,
				struct b6_tref **tref)
{
	b6_precond(tree);
	b6_precond(tref);
	return tree->ops->chk(tree, tref);
}

#endif /* B6_TREE_H_ */
