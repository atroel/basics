/*
 * Copyright (c) 2014-2015, Arnaud TROEL
 * See LICENSE file for license details.
 */

#ifndef B6_OBSERVER_H_
#define B6_OBSERVER_H_

#include "list.h"

/**
 * @file observer.h
 * @brief Helper to implement the observer design pattern.
 *
 * Given a struct example that would support foo() and bar(int) observations,
 * observers can be defined as follows:
 *
 * @code
 * struct example {
 *   ...
 *   struct b6_list observers;
 *   ...
 * };
 *
 * struct example_observer {
 *   struct b6_dref dref;
 *   const struct example_observer_ops *ops;
 * };
 *
 * struct example_observer_ops {
 *   void (*foo)(struct example_observer*);
 *   void (*bar)(struct example_observer*, int);
 * };
 *
 * static inline void reset_example_observer(struct example_observer *obs)
 * {
 *   b6_reset_observer(&obs->dref);
 * }
 *
 * static inline void add_example_observer(struct example *self,
 *                                         struct example_observer *obs)
 * {
 *   b6_attach_observer(&self->observers, &obs->dref);
 * }
 *
 * static inline void remove_example_observer(struct example_observer *obs)
 * {
 *   b6_detach_observer(&obs->dref);
 * }
 * @endcode
 *
 * On the implementation side, given that the observer contains dref and ops
 * fields as in example_observer, notification code can be generated like this:
 *
 * @code
 * static void notify_example_foo(const struct example *self)
 * {
 *   b6_notify_observers(&self->observers, example_observer, foo);
 * }
 *
 * static void notify_example_bar(const struct example *self, int param)
 * {
 *   b6_notify_observers(&self->observers, example_observer, bar, param);
 * }
 * @endcode
 */

/**
 * @brief Initialize an observer.
 *
 * This function must be called before any other. It is unsafe to detach an
 * observer using this function.
 *
 * @param dref specifies the observer double reference.
 */
static inline void b6_reset_observer(struct b6_dref *dref)
{
	dref->ref[0] = NULL;
}

/**
 * @brief Check if an observer is attached.
 * @param dref specifies the observer double reference.
 * @return true when the observer is attached.
 */
static inline int b6_observer_is_attached(const struct b6_dref *dref)
{
	return !!dref->ref[0];
}

/**
 * @brief Attach an observer.
 * @pre The observer must not be already attached.
 * @param list specifies the observers list to attach the observer to.
 * @param dref specifies the observer double reference.
 */
static inline void b6_attach_observer(struct b6_list *list,
				      struct b6_dref *dref)
{
	b6_precond(!b6_observer_is_attached(dref));
	b6_list_add_last(list, dref);
}

/**
 * @brief Detach an observer.
 * @pre The observer must be attached.
 * @param dref specifies the observer double reference.
 */
static inline void b6_detach_observer(struct b6_dref *dref)
{
	b6_precond(b6_observer_is_attached(dref));
	b6_list_del(dref);
	b6_reset_observer(dref);
}

/**
 * @brief Generate code to notify observers.
 *
 * For this generator to work, the observer struct must contain a double
 * reference named dref and a pointer the a virtual function table named ops.
 *
 * To avoid code duplication, it is better to use this macro to define the body
 * of a static function.
 *
 * @param _list specifies a pointer to the observers list.
 * @param _type specifies the name of the observer struct.
 * @param _op specifies the virtual function to call.
 * @param _args specifies the arguments of the function.
 */
#define b6_notify_observers(_list, _type, _op, _args...) \
	do { \
		const struct b6_list *_d = (_list); \
		struct b6_dref *_c = b6_list_first(_d); \
		struct b6_dref *_t = b6_list_tail(_d); \
		while (_c != _t) {\
			struct b6_dref *_n = b6_list_walk(_c, B6_NEXT); \
			struct _type *_o = b6_cast_of(_c, struct _type, dref); \
			if (_o->ops->_op) \
				_o->ops->_op(_o, ##_args); \
		       	_c = _n; \
		} \
	} while (0)

#endif /* B6_OBSERVER_H_ */
