/*
 * Copyright (c) 2014, Arnaud TROEL
 * See LICENSE file for license details.
 */

/**
 * @file event.h
 * @brief Defer code execution later in time.
 */

#ifndef B6_EVENT_H_
#define B6_EVENT_H_

#include "assert.h"
#include "heap.h"

/**
 * @brief Queue of deferred events.
 */
struct b6_event_queue {
	struct b6_heap heap;
	struct b6_array array;
	unsigned long long int shift;
	unsigned long long int time;
};

/**
 * @brief Base polymorphic structure to defer code execution in time.
 *
 * Three hooks are available to have code executed when the event is deferred
 * and when it triggers or when it is canceled.
 *
 * @code
 * struct my_event {
 *   struct b6_event up;
 *   const char *name;
 *   ...
 * };
 *
 * void my_defer(struct b6_event *up)
 * {
 *   struct my_event *self = b6_cast_of(up, struct my_event, up);
 *   printf("%s %s\n", __func__, self->name);
 * }
 *
 * void my_trigger(struct b6_event *up)
 * {
 *   struct my_event *self = b6_cast_of(up, struct my_event, up);
 *   printf("%s %s\n", __func__, self->name);
 *   free(self);
 * }
 *
 * void my_cancel(struct b6_event *up)
 * {
 *   struct my_event *self = b6_cast_of(up, struct my_event, up);
 *   printf("%s %s\n", __func__, self->name);
 *   free(self);
 * }
 *
 * static const struct event_ops my_event_ops = {
 *   .defer = my_defer,
 *   .trigger = my_trigger,
 *   .cancel = my_cancel,
 * };
 *
 * void my_event_example(struct b6_event_queue *queue)
 * {
 *   struct my_event *self = malloc(sizeof(*self));
 *   self->ops = &my_event_ops;
 *   self->name = "example";
 *   b6_defer_event(&self->up, queue, 1234);
 * }
 * @endcode
 *
 */
struct b6_event {
	const struct b6_event_ops *ops;
	unsigned long long int time;
	unsigned long int index;
};

struct b6_event_ops {
	void (*defer)(struct b6_event*);
	void (*trigger)(struct b6_event*);
	void (*cancel)(struct b6_event*);
};

/**
 * @brief Initialize an event.
 * @param self specifies the event.
 * @param ops specifies the event virtual functions.
 */
static inline void b6_reset_event(struct b6_event *self,
				  const struct b6_event_ops *ops)
{
	self->ops = ops;
	self->index = ~0UL;
}

/**
 * @brief Check if an event has been deferred.
 * @param self specifies the event.
 * @return true when the event is pending.
 */
static inline int b6_event_is_pending(struct b6_event *self)
{
	return self->index != ~0UL;
}

extern int b6_compare_event(void *lhs, void *rhs);

extern void b6_set_event_index(void *ptr, unsigned long int index);

/**
 * @brief Initialize an event queue.
 * @param self specifies the event queue.
 * @param allocator specifies the memory allocator to use to allocate the
 * underlying array of pointers.
 */
static inline void b6_initialize_event_queue(struct b6_event_queue *self,
					     struct b6_allocator *allocator)
{
	self->time = 0;
	b6_array_initialize(&self->array, allocator, sizeof(struct b6_event*));
	b6_heap_reset(&self->heap, &self->array, b6_compare_event,
		      b6_set_event_index);
}

/**
 * @brief Release resources of an event queue.
 * @param self specifies the event queue.
 */
static inline void b6_finalize_event_queue(struct b6_event_queue *self)
{
	b6_array_finalize(&self->array);
}

/**
 * @brief Postpone all events from a queue.
 * @param self specifies the event queue.
 * @param duration specifies the delay in microseconds.
 */
static inline void b6_postpone_all_events(struct b6_event_queue *self,
					  unsigned long long int duration)
{
	self->shift += duration;
}

/**
 * @brief Remove an event from an event queue.
 *
 * The cancel virtual function of the event is called if the event supports it.
 *
 * @pre The event must be pending.
 * @param self specifies the event queue.
 * @param event specifies the event.
 */
static inline void b6_cancel_event(struct b6_event_queue *self,
				   struct b6_event *event)
{
	b6_precond(b6_event_is_pending(event));
	b6_heap_extract(&self->heap, event->index);
	if (event->ops->cancel)
		event->ops->cancel(event);
	event->index = ~0UL;
}

/**
 * @brief Add an event to an event queue.
 *
 * The defer virtual function of the event is called if the event supports it.
 *
 * @pre The event must not be pending.
 * @param self specifies the event queue.
 * @param event specifies the event.
 * @param time specifies when the event should trigger (in microseconds). If the
 * event is deferred to trigger in the past, it will trigger ASAP.
 */
static inline void b6_defer_event(struct b6_event_queue *self,
				  struct b6_event *event,
				  unsigned long long int time)
{
	b6_precond(!b6_event_is_pending(event));
	b6_assert(event->ops);
	if (b6_heap_empty(&self->heap))
		self->shift = 0;
	event->time = time;
	if (event->ops->defer)
		event->ops->defer(event);
	if (event->time > self->shift)
		event->time -= self->shift;
	else
		event->time = 0;
	b6_heap_push(&self->heap, event);
}

/**
 * @brief Empty an event queue.
 * @param self specifies the event queue.
 */
extern void b6_cancel_all_events(struct b6_event_queue *self);

/**
 * @brief Trigger events from an event queue.
 * @param self specifies the event queue.
 * @param now specifies the current time in microseconds. Every event which is
 * programmed to trigger before will have its virtual function called and will
 * be removed from the queue.
 */
extern void b6_trigger_events(struct b6_event_queue *self,
			      unsigned long long int now);

#endif /* B6_EVENT_H_ */
