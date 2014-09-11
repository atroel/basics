#include <stdio.h>
#include <stdlib.h>

void b6_assert_handler(const char *func, const char *file, int line, int type,
                       const char *condition)
{
	fprintf(stderr, "%s (%s:%d): %s failed\n", func, file, line, condition);
	abort();
}
