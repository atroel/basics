/*
 * Copyright (c) 2009-2015, Arnaud TROEL
 * See LICENSE file for license details.
 */

#ifndef ASSERT_H_
#define ASSERT_H_

#include "utils.h"

extern void b6_assert_handler(const char *func, const char *file, int line,
                              int type, const char *condition);

#define __B6_CHECK(type, condition)					\
	do if (b6_unlikely(!(condition)))				\
		b6_assert_handler(__func__, __FILE__, __LINE__, type,	\
		                  #condition);				\
	while(0)

#define __B6_ASSERT(type, condition)					\
	do if (b6_unlikely(!(condition)))				\
		b6_assert_handler(__func__, __FILE__, __LINE__, type,	\
		                  #condition);				\
	while(0)

#define b6_static_assert(condition)					\
	do switch (0) {							\
	case (0 == 1):							\
	case (condition):						\
	default:							\
		break;							\
	} while (0)

enum { B6_CHECK, B6_ASSERT, B6_PRECOND, B6_POSTCOND };

#define b6_check(condition)	__B6_CHECK(B6_CHECK, condition)
#ifndef NDEBUG
#define b6_assert(condition)	__B6_CHECK(B6_ASSERT, condition)
#define b6_precond(condition)	__B6_CHECK(B6_PRECOND, condition)
#define b6_postcond(condition)	__B6_CHECK(B6_POSTCOND, condition)
#else /*NDEBUG*/
#define b6_assert(condition) (void)(condition)
#define b6_precond(condition) (void)(condition)
#define b6_postcond(condition) (void)(condition)
#endif /*NDEBUG*/

#endif /*ASSERT_H_*/
