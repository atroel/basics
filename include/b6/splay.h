/*
 * Copyright (c) 2010, Arnaud TROEL
 * See LICENSE file for license details.
 */

#ifndef B6_SPLAY_H_
#define B6_SPLAY_H_

#include "refs.h"
#include "utils.h"
#include "assert.h"

/**
 * @file splay.h
 *
 * @brief Splay tree container
 *
 * Splay trees are self-balanced binary search trees where recently accessed
 * elements are naturally moved next to the root. As such, there is no need of
 * an extra balance parameter in reference. Thus, they have a small footprint
 * in memory compared to other trees since they only make use of double
 * references.
 *
 * This implementation proposes threaded splay trees as an option so that
 * in-order traversal is possible without any comparison and despite the lack
 * of top reference. Eventually insertion and deletion a bit slower,
 * however. Note that in-order traversal using b6_splay_walk does not move any
 * element within the tree.
 */

/**
 * @brief splay tree
 */
struct b6_splay {
	struct b6_dref dref; /**< sentinel */
};

/**
 * @internal
 */
#define __b6_splay_root(splay) (splay)->dref.ref[0]

/**
 * @internal
 */
static inline int __b6_splay_is_thread(const struct b6_dref *dref)
{
	return (unsigned long int)dref & 1;
}

/**
 * @internal
 */
static inline struct b6_dref *__b6_splay_to_thread(const struct b6_dref *dref)
{
	return (struct b6_dref *)((unsigned long int)dref | 1);
}

/**
 * @internal
 */
static inline struct b6_dref *__b6_splay_from_thread(const struct b6_dref *dref)
{
	return (struct b6_dref *)((unsigned long int)dref & ~1);
}

/**
 * @brief Initialize or clear a splay tree
 * @complexity O(1)
 * @param splay pointer to the splay tree
 */
static inline void b6_splay_initialize(struct b6_splay *splay)
{
	__b6_splay_root(splay) = NULL;
}

/**
 * @brief Return the reference of the most recently accessed reference in the
 * splay tree
 * @complexity O(1)
 * @param splay pointer to the splay tree
 * @return pointer to the root reference of the splay tree
 */
static inline struct b6_dref *b6_splay_root(const struct b6_splay *splay)
{
	return __b6_splay_root(splay);
}


/**
 * @brief Test if a splay tree contains elements
 * @complexity O(1)
 * @param splay pointer to the splay tree
 * @return 0 if the splay tree contains one element or more and another value
 * otherwise
 */
static inline int b6_splay_empty(const struct b6_splay *splay)
{
	struct b6_dref *root = b6_splay_root(splay);
	return (!root) | __b6_splay_is_thread(root);
}

/**
 * @brief Insert a new element in a splay tree
 * @complexity O(1)
 * @param splay pointer to the splay tree
 * @param ref reference of the element to insert
 * @param dir B6_PREV/B6_NEXT if the element to insert is smaller/greater than
 * the root of the splay tree
 * @return ref
 */
static inline struct b6_dref *b6_splay_add_nothread(struct b6_splay *splay,
						    int dir,
						    struct b6_dref *ref)
{
	if (b6_splay_empty(splay)) {
		struct b6_dref *top = b6_splay_root(splay);
		int opp = b6_to_opposite(dir);
		ref->ref[opp] = top;
		ref->ref[dir] = top->ref[dir];
		top->ref[dir] = NULL;
	} else
		ref->ref[B6_NEXT] = ref->ref[B6_PREV] = NULL;

	__b6_splay_root(splay) = ref;
	return ref;
}

/**
 * @internal
 */
extern struct b6_dref *__b6_splay_swap_nothread(struct b6_dref *);

/**
 * @brief Remove the element at the root of the splay tree
 * @complexity O(1)
 * @param splay pointer to the splay tree
 * @return pointer to the reference of the element removed
 */
static inline struct b6_dref *b6_splay_del_nothread(struct b6_splay *splay)
{
	struct b6_dref *ref, *top = b6_splay_root(splay);

	b6_precond(!b6_splay_empty(splay));

	if (!(top->ref[B6_PREV]))
		ref = top->ref[B6_NEXT];
	else if (!(top->ref[B6_NEXT]))
		ref = top->ref[B6_PREV];
	else {
		ref = __b6_splay_swap_nothread(top->ref[B6_NEXT]);
		ref->ref[B6_PREV] = top->ref[B6_PREV];
	}

	__b6_splay_root(splay) = ref;
	return top;
}

#define b6_splay_search_nothread(_splay, _dir, _cmp, _arg)		\
	( {								\
		struct b6_dref bak, *lnk[] = { &bak, &bak };		\
		struct b6_dref *swp, *top = b6_splay_root(_splay);	\
		int dir = B6_NEXT, opp = B6_PREV, res = 1, tmp;		\
									\
		if (b6_splay_empty(_splay))				\
			goto done;					\
									\
		for (res = _cmp(top, _arg); res; top = top->ref[dir]) { \
			opp = b6_to_direction(res);			\
			dir = b6_to_opposite(opp);			\
			if (!top->ref[dir])				\
				break;					\
									\
			tmp = res;					\
			res = _cmp(top->ref[dir], _arg);		\
			if (res == tmp) {				\
				swp = top->ref[dir];			\
				top->ref[dir] = swp->ref[opp];		\
				swp->ref[opp] = top;			\
				top = swp;				\
				if (!top->ref[dir])			\
					break;				\
				res = _cmp(top->ref[dir], _arg);	\
			}						\
									\
			lnk[opp]->ref[dir] = top;			\
			lnk[opp] = top;					\
		}							\
									\
		lnk[B6_NEXT]->ref[B6_PREV] = top->ref[B6_NEXT];		\
		lnk[B6_PREV]->ref[B6_NEXT] = top->ref[B6_PREV];		\
		top->ref[B6_PREV] = bak.ref[B6_NEXT];			\
		top->ref[B6_NEXT] = bak.ref[B6_PREV];			\
									\
		__b6_splay_root(_splay) = top;				\
	done:								\
		(_dir) = dir;						\
		res;							\
	} )

/**
 * @brief Return the reference before the smallest element of the splay tree
 * @complexity O(log(N))
 * @param splay pointer to the splay tree
 * @return pointer to the reference of the head of the container
 *
 * The head reference is such that there is no previous reference in the
 * container. It cannot be dereferenced as it is associated with no element.
 */
static inline struct b6_dref *b6_splay_head(const struct b6_splay *splay)
{
	b6_precond(splay);
	return (struct b6_dref *)&splay->dref;
}

/**
 * @brief Return the reference before the greatest element of the splay tree
 * @complexity O(log(N))
 * @param splay pointer to the splay tree
 * @return pointer to the reference of the tail of the container
 *
 * The tail reference is such that there is no next reference in the
 * container. It cannot be dereferenced as it is associated with no element.
 */
static inline struct b6_dref *b6_splay_tail(const struct b6_splay *splay)
{
	return b6_splay_head(splay);
}

/**
 * @internal
 */
extern struct b6_dref *__b6_splay_dive(struct b6_dref *, int);

/**
 * @brief In-order traveling of a splay tree
 * @complexity O(log(n))
 * @param splay pointer to the splay tree
 * @param ref reference to walk from
 * @param dir B6_PREV (B6_NEXT) to get the reference of the greatest
 * smaller elements (smallest greater elements respectively)
 * @return the next or previous reference in the sequence or head if the splay
 * tree is not threaded
 */
static inline struct b6_dref *b6_splay_walk(const struct b6_splay *splay,
					    struct b6_dref *dref, int dir)
{
	if (b6_unlikely(dref == b6_splay_head(splay)))
		if (b6_splay_empty(splay))
			return dref;
		else
			return __b6_splay_dive(b6_splay_root(splay),
					       b6_to_opposite(dir));
	else if (__b6_splay_is_thread(dref->ref[dir]))
		return __b6_splay_from_thread(dref->ref[dir]);
	else
		return __b6_splay_dive(dref->ref[dir], b6_to_opposite(dir));
}

/**
 * @brief Return the reference of the smallest element in the splay tree
 * according to how elements compare within it
 * @complexity O(log(N))
 * @param splay pointer to the splay tree
 * @return pointer to the reference of the smallest element or tail if the
 * tree is empty or if it does not support threads
 */
static inline struct b6_dref *b6_splay_first(const struct b6_splay *splay)
{
	return b6_splay_walk(splay, b6_splay_head(splay), B6_NEXT);
}

/**
 * @brief Return the reference of the greatest element in the splay tree
 * according to how elements compare within it
 * @complexity O(1og(N))
 * @param splay pointer to the splay tree
 * @return pointer to the reference of the greatest element or head if the
 * tree is empty or if it does not support threads
 */
static inline struct b6_dref *b6_splay_last(const struct b6_splay *splay)
{
	return b6_splay_walk(splay, b6_splay_tail(splay), B6_PREV);
}

static inline struct b6_dref *b6_splay_add(struct b6_splay *splay, int dir,
					   struct b6_dref *ref)
{
	if (!b6_splay_empty(splay)) {
		int opp = b6_to_opposite(dir);
		struct b6_dref *top = b6_splay_root(splay);
		struct b6_dref *tmp = top->ref[dir];
		ref->ref[opp] = top;
		ref->ref[dir] = tmp;
		top->ref[dir] = __b6_splay_to_thread(ref);
		if (!__b6_splay_is_thread(tmp)) {
			tmp = __b6_splay_dive(tmp, opp);
			tmp->ref[opp] = __b6_splay_to_thread(ref);
		}
	} else  {
		struct b6_dref *head = b6_splay_head(splay);
		ref->ref[B6_PREV] = __b6_splay_to_thread(head);
		ref->ref[B6_NEXT] = __b6_splay_to_thread(head);
	}

	__b6_splay_root(splay) = ref;
	return ref;
}

/**
 * @internal
 */
extern struct b6_dref *__b6_splay_swap(struct b6_dref *);

static inline struct b6_dref *b6_splay_del(struct b6_splay *splay)
{
	struct b6_dref *ref, *tmp, *top = b6_splay_root(splay);

	b6_precond(!b6_splay_empty(splay));

	if (__b6_splay_is_thread(top->ref[B6_PREV])) {
		ref = top->ref[B6_NEXT];
		if (!__b6_splay_is_thread(ref)) {
			tmp = __b6_splay_dive(ref, B6_PREV);
			tmp->ref[B6_PREV] = top->ref[B6_PREV];
		} else
			ref = NULL;
		goto done;
	}

	if (__b6_splay_is_thread(top->ref[B6_NEXT])) {
		ref = top->ref[B6_PREV];
		tmp = __b6_splay_dive(ref, B6_NEXT);
		tmp->ref[B6_NEXT] = top->ref[B6_NEXT];
		goto done;
	}

	ref = __b6_splay_swap(top->ref[B6_NEXT]);
	ref->ref[B6_PREV] = top->ref[B6_PREV];
	tmp = __b6_splay_dive(ref->ref[B6_PREV], B6_NEXT);
	tmp->ref[B6_NEXT] = __b6_splay_to_thread(ref);

done:
	__b6_splay_root(splay) = ref;
	return top;
}

/**
 * @brief Search a splay tree
 * @param splay pointer to the splay tree
 * @param dir
 * @param cmp
 * @param arg opaque data to pass to examine if not NULL or a pointer to
 * the reference of an element to find an equal in splay.
 * @return true/false
 */
#define b6_splay_search(_splay, _dir, _cmp, _arg)			\
	( {								\
		struct b6_dref bak, *lnk[] = { &bak, &bak };		\
		struct b6_dref *swp, *top = b6_splay_root(_splay);	\
		int dir = B6_NEXT, opp = B6_PREV, res = 1, tmp;		\
									\
		if (b6_splay_empty(_splay))				\
			goto done;					\
									\
		for (res = _cmp(top, _arg); res; top = top->ref[dir]) { \
			opp = b6_to_direction(res);			\
			dir = b6_to_opposite(opp);			\
			if (__b6_splay_is_thread(top->ref[dir]))	\
				break;					\
									\
			tmp = res;					\
			res = _cmp(top->ref[dir], _arg);		\
			if (res == tmp) {				\
				swp = top->ref[dir];			\
				if (__b6_splay_is_thread(swp->ref[opp])) \
					top->ref[dir] =			\
						__b6_splay_to_thread(swp); \
				else					\
					top->ref[dir] = swp->ref[opp];	\
				swp->ref[opp] = top;			\
				top = swp;				\
				if (__b6_splay_is_thread(top->ref[dir])) \
					break;				\
				res = _cmp(top->ref[dir], _arg);	\
			}						\
									\
			lnk[opp]->ref[dir] = top;			\
			lnk[opp] = top;					\
		}							\
									\
		if (__b6_splay_to_thread(lnk[opp]) != top->ref[opp])	\
			lnk[opp]->ref[dir] = top->ref[opp];		\
		else							\
			lnk[opp]->ref[dir] = __b6_splay_to_thread(top); \
		lnk[dir]->ref[opp] = top->ref[dir];			\
		top->ref[B6_PREV] = bak.ref[B6_NEXT];			\
		top->ref[B6_NEXT] = bak.ref[B6_PREV];			\
									\
		__b6_splay_root(_splay) = top;				\
	done:								\
		(_dir) = dir;						\
		res;							\
	} )

#endif /* B6_SPLAY_H_ */
