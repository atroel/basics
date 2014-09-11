#ifndef _TEST_H
#define _TEST_H

#include <stdio.h>
#include <setjmp.h>

extern jmp_buf *test_handler;
extern unsigned test_passed;
extern unsigned test_failed;

void b6_assert_handler(const char *func, const char *file, int line, int type,
                       const char *condition);

void test_init(void);
void test_exit(void);

#define test_exec(func, params) {					\
	int _i;								\
	jmp_buf _env;							\
	printf(" %s\r", #func);						\
	_i = setjmp(_env);						\
	if (!_i)  {							\
		test_handler = &_env;					\
		_i = func(params);					\
	} else								\
		_i = 0;							\
	printf("\t\t\t\t\t\t\t\t       [%s]\n",				\
	       _i ? "  OK  " : "FAILED");				\
	test_passed += !!_i;						\
	test_failed += !_i;						\
}

#endif /* _TEST_H */
