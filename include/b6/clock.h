/*
 * Copyright (c) 2014-2015, Arnaud TROEL
 * See LICENSE file for license details.
 */

/**
 * @file clock.h
 * @brief Get time in microseconds or wait for some delay.
 */

#ifndef B6_CLOCK_H_
#define B6_CLOCK_H_

#include "registry.h"

/**
 * @brief Base polymorphic structure to deal with running time.
 */
struct b6_clock {
	const struct b6_clock_ops *ops; /**< pointer to virtual methods */
};

struct b6_clock_ops {
	unsigned long long int (*get_time)(const struct b6_clock*);
	void (*wait)(const struct b6_clock*, unsigned long long int);
};

/**
 * @brief Read time from a clock.
 * @param c specifies the clock.
 * @return the clock time in microseconds.
 */
static inline unsigned long long int b6_get_clock_time(const struct b6_clock *c)
{
	return c->ops->get_time(c);
}

/**
 * @brief Wait for a clock for some time.
 * @param self specifies the clock.
 * @param delay_us specifies the amount of time in microseconds to wait.
 */
static inline void b6_wait_clock(const struct b6_clock* self,
				 unsigned long long int delay_us)
{
	self->ops->wait(self, delay_us);
}

/**
 * @brief A fake clock implementation useful for testing purpose.
 *
 * A fake clock can be passed as a clock to test clock-based APIs:
 *
 * @code
 * void do_some_time_based_stuff(struct clock *clock);
 *
 * int test_some_time_based_stuff(void)
 * {
 *   struct b6_fake_clock fake;
 *   b6_reset_fake_clock(&fake);
 *   b6_set_fake_time(&fake, 1000 * 1000);
 *   do_some_time_based_stuff(&fake.up);
 *   b6_advance_fake_time(&fake, 5 * 1000 * 1000);
 *   do_some_time_based_stuff(&fake.up);
 *   return stuff_is_ok();
 * }
 * @endcode
 */
struct b6_fake_clock {
	struct b6_clock up;
	unsigned long long int time;
};

/**
 * @internal
 */
extern const struct b6_clock_ops b6_fake_clock_ops;

/**
 * @brief Initialize a fake clock.
 *
 * The time of the fake clock is initialized to zero.
 *
 * @param self specifies the fake clock.
 * @param time specifies the new current time in microseconds of the fake clock.
 */
static inline void b6_reset_fake_clock(struct b6_fake_clock *self,
				       unsigned long long int time)
{
	self->up.ops = &b6_fake_clock_ops;
	self->time = time;
}

/**
 * @brief Read the current time of a fake clock.
 *
 * This returns the same value as: `b6_get_clock_time(&fake_clock.up);`
 *
 * @param self specifies the fake clock.
 * @return the current time of the fake clock in microseconds.
 */
static inline unsigned long long int b6_get_fake_clock_time(
	const struct b6_fake_clock *self)
{
	return self->time;
}

/**
 * @brief Advance the current time of a fake clock.
 * @param self specifies the fake clock.
 * @param duration specifies how many microseconds to add to the current time.
 */
static inline void b6_wait_fake_clock(struct b6_fake_clock *self,
				      unsigned long long int duration)
{
	self->time += duration;
}

/**
 * @brief A clock decorator to return constant time over several subsequent
 * `b6_get_clock_time` calls.
 *
 * In the code below, the time is updated at the beginning of the loop only.
 * Both functions do_some_stuff and do_some_more_stuff get the same current time
 * value when they call `b6_get_clock_time`, although time is still running.
 *
 * Note that calls to b6_wait_clock synchonize the cached time and its base
 * clock.
 *
 * @code
 * void foo(struct clock *base)
 * {
 *   struct b6_cached_clock clock;
 *   b6_setup_cached_clock(&clock, base);
 *   for (;;) {
 *     b6_sync_cached_clock(&clock);
 *     do_some_stuff(&clock.up);
 *     do_some_more_stuff(&clock.up);
 *   }
 * }
 * @endcode
 */
struct b6_cached_clock {
	struct b6_clock up;
	const struct b6_clock *clock;
	unsigned long long int time;
};

/**
 * @internal
 */
extern const struct b6_clock_ops b6_cached_clock_ops;

/**
 * @brief Initialize a cached clock to rely on a base clock.
 * @param self specifies the cached clock.
 * @param clock specifies the base clock to sync against.
 */
static inline void b6_setup_cached_clock(struct b6_cached_clock *self,
					 const struct b6_clock *clock)
{
	self->up.ops = &b6_cached_clock_ops;
	self->clock = clock;
	self->time = 0ULL;
}

/**
 * @brief Synchronize a cached clock with its base clock.
 * @param self specifies the cached clock.
 * @return the current time in microseconds.
 */
static inline unsigned long long int b6_sync_cached_clock(
	struct b6_cached_clock *self)
{
	return self->time = b6_get_clock_time(self->clock);
}

/**
 * @brief A clock decorator with which time can be paused and resumed.
 *
 * Can simplify implementation of pause/resume operations depending on time.
 */
struct b6_stopwatch {
	struct b6_clock up;
	const struct b6_clock *clock;
	unsigned long long int base_us; /* time when this clock was paused */
	unsigned long long int diff_us; /* time lost versus the base clock */
	int frozen; /* pause/resume count */
};

/**
 * @brief Pause a stopwatch.
 *
 * Subsequents calls to b6_get_clock_time will return the same value until
 * b6_resume_stopwatch is called.
 *
 * @param self specifies the stopwatch.
 */
static inline void b6_pause_stopwatch(struct b6_stopwatch *self)
{
	if (!self->frozen++)
		self->base_us = b6_get_clock_time(self->clock);
}

/**
 * @brief Resume a stopwatch.
 * @param self specifies the stopwatch.
 */
static inline void b6_resume_stopwatch(struct b6_stopwatch *self)
{
	if (!--self->frozen) {
		self->diff_us += b6_get_clock_time(self->clock) - self->base_us;
		self->base_us = 0ULL;
	}
}

/**
 * @brief Read the current time of a stopwatch.
 *
 * This returns the same result as: `b6_get_clock_time(&stopwatch.up);`
 *
 * @param self specifies the stopwatch.
 * @return the current time of the stopwatch in microseconds.
 */
static inline unsigned long long int b6_get_stopwatch_time(
	const struct b6_stopwatch *self)
{
	unsigned long long int time =
		self->base_us ? self->base_us : b6_get_clock_time(self->clock);
	return time - self->diff_us;
}

/**
 * @brief Wait for a stopwatch ; may wait for ever if the stopwatch is paused.
 * @param self specifies the stopwatch to wait for.
 * @param delay_us specifies the amount of time in microseconds to wait.
 */
extern void b6_wait_stopwatch(const struct b6_stopwatch *self,
			      unsigned long long int delay_us);

/**
 * @internal
 */
extern const struct b6_clock_ops b6_stopwatch_clock_ops;

/**
 * @brief Initialize a stopwatch to rely on a base clock.
 * @param self specifies the stopwatch.
 * @param clock specifies the base clock to rely on.
 */
static inline void b6_setup_stopwatch(struct b6_stopwatch *self,
				      const struct b6_clock *clock)
{
	self->up.ops = &b6_stopwatch_clock_ops;
	self->clock = clock;
	self->base_us = self->diff_us = 0ULL;
	self->frozen = 0;
}

/**
 * @internal
 */
extern struct b6_registry b6_named_clock_registry;

/**
 * @brief A named clock that can be added/removed/looked up by name.
 */
struct b6_named_clock {
	struct b6_entry entry;
	struct b6_clock *clock;
};

/**
 * @brief Register a clock under a specific name.
 * @param self specifies the named clock.
 * @param name specifies the name as which the clock should be registered.
 * @return 0 for success.
 * @return -1 if a clock has already been registered with the same name.
 */
static inline int b6_register_named_clock(struct b6_named_clock *self,
					  const char *name)
{
	return b6_register(&b6_named_clock_registry, &self->entry, name);
}

/**
 * @brief Remove a named clock from the clock register.
 * @pre The named clock must actually be registered at the time of the call.
 * @param self specifies the named clock.
 */
static inline void b6_unregister_named_clock(struct b6_named_clock *self)
{
	return b6_unregister(&b6_named_clock_registry, &self->entry);
}

/**
 * @brief Find a clock by its name.
 * @param name specifies the name of the clock.
 * @return a pointer to the named clock.
 * @return NULL if the named clock was not found.
 */
static inline struct b6_named_clock *b6_lookup_named_clock(const char *name)
{
	struct b6_entry *entry = b6_lookup_registry(&b6_named_clock_registry,
						    name);
	return entry ? b6_cast_of(entry, struct b6_named_clock, entry) : NULL;
}

/**
 * @brief Get the first named clock from the registry.
 * @return a pointer to a named clock in the registry.
 * @return NULL if no named clock has been registered.
 */
static inline struct b6_named_clock *b6_get_default_named_clock(void)
{
	struct b6_tref *tref = b6_tree_first(&b6_named_clock_registry.tree);
	struct b6_entry *entry = b6_cast_of(tref, struct b6_entry, tref);
	struct b6_named_clock *source = NULL;
	if (tref != b6_tree_tail(&b6_named_clock_registry.tree))
		source = b6_cast_of(entry, struct b6_named_clock, entry);
	return source;
}

#endif /* B6_CLOCK_H_ */
