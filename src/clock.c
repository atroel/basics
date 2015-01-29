/*
 * Copyright (c) 2014-2015, Arnaud TROEL
 * See LICENSE file for license details.
 */

#include "b6/assert.h"
#include "b6/clock.h"

B6_REGISTRY_DEFINE(b6_named_clock_registry);

static unsigned long long int get_fake_clock_time(const struct b6_clock *up)
{
	return b6_get_fake_clock_time(b6_cast_of(up, struct b6_fake_clock, up));
}

static void wait_fake_clock(const struct b6_clock *up,
			    unsigned long long int delay)
{
	b6_wait_fake_clock(b6_cast_of(up, struct b6_fake_clock, up), delay);
}

const struct b6_clock_ops b6_fake_clock_ops = {
	.get_time = get_fake_clock_time,
	.wait = wait_fake_clock,
};

void b6_wait_stopwatch(const struct b6_stopwatch *self,
		       unsigned long long int delay_us)
{
	unsigned long long int limit = b6_get_stopwatch_time(self) + delay_us;
	for (;;) {
		unsigned long long int time;
		b6_wait_clock(self->clock, delay_us);
		time = b6_get_stopwatch_time(self);
		if (time > limit)
			break;
		delay_us = limit - time;
	}
}

static unsigned long long int get_stopwatch_time(const struct b6_clock *self)
{
	return b6_get_stopwatch_time(b6_cast_of(self, struct b6_stopwatch, up));
}

static inline void wait_stopwatch(const struct b6_clock *self,
				  unsigned long long int delay_us)
{
	return b6_wait_stopwatch(b6_cast_of(self, struct b6_stopwatch, up),
				 delay_us);
}

const struct b6_clock_ops b6_stopwatch_clock_ops = {
	.get_time = get_stopwatch_time,
	.wait = wait_stopwatch,
};

static unsigned long long int get_cached_clock_time(const struct b6_clock *up)
{
	return b6_cast_of(up, struct b6_cached_clock, up)->time;
}

static inline void wait_cached_clock(const struct b6_clock *up,
				     unsigned long long int delay_us)
{
	struct b6_cached_clock *self =
		b6_cast_of(up, struct b6_cached_clock, up);
	b6_wait_clock(self->clock, delay_us);
	b6_sync_cached_clock(self);
}

const struct b6_clock_ops b6_cached_clock_ops = {
	.get_time = get_cached_clock_time,
	.wait = wait_cached_clock,
};
