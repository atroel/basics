/*
 * Copyright (c) 2010, Arnaud TROEL
 * See LICENSE file for license details.
 */

#include "b6/list.h"

unsigned long int b6_list_length(const struct b6_list *list)
{
	unsigned long int length = 0;
	const struct b6_dref *dref = b6_list_first(list);
	const struct b6_dref *tail = b6_list_tail(list);
	while (dref != tail) {
		length += 1;
		dref = b6_list_walk(dref, B6_NEXT);
	}
	return length;
}

void __b6_list_msort(struct b6_list *list, b6_compare_t comp,
		     unsigned long int length)
{
	B6_LIST_DEFINE(llist);
	B6_LIST_DEFINE(rlist);
	struct b6_dref *dref, *lref, *rref;
	unsigned long int half;
	unsigned long int llen = length / 2;
	unsigned long int rlen = length - llen;

	for (half = llen, dref = b6_list_first(list); --half;
	     dref = b6_list_walk(dref, B6_NEXT));

	b6_list_move(b6_list_first(list), dref, b6_list_tail(&llist));
	b6_list_move(b6_list_first(list), b6_list_last(list),
		     b6_list_tail(&rlist));

	if (llen > 1)
		__b6_list_msort(&llist, comp, llen);

	if (rlen > 1)
		__b6_list_msort(&rlist, comp, rlen);

	lref = b6_list_first(&llist);
	rref = b6_list_first(&rlist);
	if (comp(lref, rref) >= 0)
		goto shift;

merge:
	dref = rref;
	do {
		dref = b6_list_walk(dref, B6_NEXT);
		if (--rlen)
			continue;
		b6_list_move(rref, b6_list_last(&rlist), b6_list_tail(list));
		b6_list_move(lref, b6_list_last(&llist), b6_list_tail(list));
		return;
	} while (comp(dref, lref) >= 0);
	b6_list_move(rref, b6_list_walk(dref, B6_PREV), b6_list_tail(list));
	rref = dref;

shift:
	dref = lref;
	do {
		dref = b6_list_walk(dref, B6_NEXT);
		if (--llen)
			continue;
		b6_list_move(lref, b6_list_last(&llist), b6_list_tail(list));
		b6_list_move(rref, b6_list_last(&rlist), b6_list_tail(list));
		return;
	} while (comp(dref, rref) >= 0);
	b6_list_move(lref, b6_list_walk(dref, B6_PREV), b6_list_tail(list));
	lref = dref;

	goto merge;
}

void b6_list_qsort(struct b6_list *list, b6_compare_t comp)
{
	struct b6_dref *pivot, *prev, *next, *dref;

	next = pivot = b6_list_first(list);
	dref = b6_list_last(list);
	do {
		prev = next;
		next = b6_list_walk(prev, B6_NEXT);
		if (comp(dref, prev) >= 0)
			continue;
		if (pivot != prev) {
			b6_list_del(prev);
			b6_list_add(pivot, prev);
			b6_list_del(pivot);
			b6_list_add(next, pivot);
		}
		pivot = b6_list_walk(prev, B6_NEXT);
	} while (prev != dref);

	if (pivot != dref) {
		next = b6_list_walk(dref, B6_NEXT);
		b6_list_del(dref);
		b6_list_add(pivot, dref);
		b6_list_del(pivot);
		b6_list_add(next, pivot);
	}
	pivot = prev;

	prev = b6_list_walk(pivot, B6_PREV);
	dref = b6_list_first(list);
	if (pivot != dref && prev != dref) {
		B6_LIST_DEFINE(temp);
		b6_list_move(dref, prev, b6_list_tail(&temp));
		b6_list_qsort(&temp, comp);
		b6_list_move(b6_list_first(&temp), b6_list_last(&temp), pivot);
	}

	next = b6_list_walk(pivot, B6_NEXT);
	dref = b6_list_last(list);
	if (pivot != dref && next != dref) {
		B6_LIST_DEFINE(temp);
		b6_list_move(next, dref, b6_list_tail(&temp));
		b6_list_qsort(&temp, comp);
		b6_list_move(b6_list_first(&temp), b6_list_last(&temp),
			     b6_list_tail(list));
	}
}
