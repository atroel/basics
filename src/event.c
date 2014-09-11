/*
 * Copyright (c) 2014, Arnaud TROEL
 * See LICENSE file for license details.
 */

#include "b6/event.h"

void b6_cancel_all_events(struct b6_event_queue *self)
{
	while (!b6_heap_empty(&self->heap))
		b6_cancel_event(self, b6_heap_top(&self->heap));
}

void b6_trigger_events(struct b6_event_queue *self, unsigned long long int now)
{
	self->time = now;
	while (!b6_heap_empty(&self->heap)) {
		struct b6_event *event = b6_heap_top(&self->heap);
		if (event->time > self->time - self->shift)
			break;
		b6_heap_pop(&self->heap);
		event->time += self->shift;
		event->index = ~0UL;
		if (event->ops->trigger)
			event->ops->trigger(event);
	}
}

int b6_compare_event(void *lhs, void *rhs)
{
	const struct b6_event *l = lhs;
	const struct b6_event *r = rhs;
	if (l->time < r->time)
		return -1;
	if (l->time > r->time)
		return 1;
	return 0;
}

void b6_set_event_index(void *ptr, unsigned long int index)
{
	((struct b6_event*)ptr)->index = index;
}
