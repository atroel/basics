#include "test.h"

jmp_buf *test_handler;
unsigned test_passed = 0;
unsigned test_failed = 0;

void test_init(void)
{
	test_handler = NULL;
	test_passed = 0;
	test_failed = 0;
}


void test_exit(void)
{
	unsigned passed = test_passed;
	unsigned failed = test_failed;

	printf("\nPassed %u of %u (%f%%)\n", passed, passed + failed,
	       100. * passed / (passed + failed));
}

void b6_assert_handler(const char *func, const char *file, int line, int type,
                       const char *condition)
{
	longjmp(*test_handler, 1);
}
