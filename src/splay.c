/*
 * Copyright (c) 2010, Arnaud TROEL
 * See LICENSE file for license details.
 */
#include "b6/splay.h"

struct b6_dref *__b6_splay_swap_nothread(struct b6_dref *ref)
{
	struct b6_dref bak = { { NULL, NULL } };
	struct b6_dref *lnk[] = { &bak, &bak };

	while (ref->ref[B6_PREV]) {
	 	struct b6_dref *tmp = ref->ref[B6_PREV];
		ref->ref[B6_PREV] = tmp->ref[B6_NEXT];
		tmp->ref[B6_NEXT] = ref;
		ref = tmp;
		if (!ref->ref[B6_PREV])
			break;
		lnk[B6_NEXT]->ref[B6_PREV] = ref;
		lnk[B6_NEXT] = ref;
		ref = ref->ref[B6_PREV];
	}

	lnk[B6_NEXT]->ref[B6_PREV] = ref->ref[B6_NEXT];
	lnk[B6_PREV]->ref[B6_NEXT] = ref->ref[B6_PREV];
	ref->ref[B6_PREV] = bak.ref[B6_NEXT];
	ref->ref[B6_NEXT] = bak.ref[B6_PREV];

	return ref;
}

struct b6_dref *__b6_splay_swap(struct b6_dref *ref)
{
	struct b6_dref bak = { { NULL, NULL } };
	struct b6_dref *lnk[] = { &bak, &bak };

	while (!__b6_splay_is_thread(ref->ref[B6_PREV])) {
	 	struct b6_dref *tmp = ref->ref[B6_PREV];
		if (__b6_splay_is_thread(tmp->ref[B6_NEXT]))
			ref->ref[B6_PREV] = __b6_splay_to_thread(tmp);
		else
			ref->ref[B6_PREV] = tmp->ref[B6_NEXT];
		tmp->ref[B6_NEXT] = ref;
		ref = tmp;
		if (__b6_splay_is_thread(ref->ref[B6_PREV]))
			break;
		lnk[B6_NEXT]->ref[B6_PREV] = ref;
		lnk[B6_NEXT] = ref;
		ref = ref->ref[B6_PREV];
	}

	if (__b6_splay_to_thread(lnk[B6_NEXT]) != ref->ref[B6_NEXT])
		lnk[B6_NEXT]->ref[B6_PREV] = ref->ref[B6_NEXT];
	else
		lnk[B6_NEXT]->ref[B6_PREV] = __b6_splay_to_thread(ref);
	lnk[B6_PREV]->ref[B6_NEXT] = ref->ref[B6_PREV];
	ref->ref[B6_PREV] = bak.ref[B6_NEXT];
	ref->ref[B6_NEXT] = bak.ref[B6_PREV];

	return ref;
}

struct b6_dref *__b6_splay_dive(struct b6_dref *ref, int dir)
{
	struct b6_dref *tmp;

	while (!__b6_splay_is_thread(tmp = ref->ref[dir]))
		ref = tmp;

	return ref;
}
