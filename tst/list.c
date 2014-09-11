#include "b6/list.h"
#include "test.h"

static int always_fails()
{
	return 0;
}

static int static_init()
{
	B6_LIST_DEFINE(list);

	return b6_list_empty(&list);
}

static int runtime_init()
{
	struct b6_list list;

	b6_list_initialize(&list);

	return b6_list_empty(&list);
}

static int last_is_head_when_empty()
{
	B6_LIST_DEFINE(list);

	return b6_list_last(&list) == b6_list_head(&list);
}

static int first_is_tail_when_empty()
{
	B6_LIST_DEFINE(list);

	return b6_list_first(&list) == b6_list_tail(&list);
}

/* Not valid anymore since head and tail have merged
static int add_before_head()
{
	B6_LIST_DEFINE(list);
	struct b6_dref dref;
	int retval;
	jmp_buf env;

	retval = setjmp(env);
	if (!retval) {
		test_handler = &env;
		b6_list_add(&list, b6_list_head(&list), &dref);
	}

	return retval;
}
*/

static int add_null()
{
	B6_LIST_DEFINE(list);
	int retval;
	jmp_buf env;

	retval = setjmp(env);
	if (!retval) {
		test_handler = &env;
		b6_list_add(b6_list_tail(&list), NULL);
	}

	return retval;
}

static int add()
{
	B6_LIST_DEFINE(list);
	struct b6_dref dref[3];
	struct b6_dref temp;
	struct b6_dref *iter;
	int i;

	for (i = 0, iter = b6_list_tail(&list); i < b6_card_of(dref); i += 1) {
		iter = b6_list_add(iter, &dref[i]);

		if (iter != &dref[i])
			return 0;

		if (b6_list_empty(&list))
			return 0;
	}

	iter = b6_list_add(&dref[0], &temp);

	if (iter != &temp)
		return 0;

	if (b6_list_empty(&list))
		return 0;

	return 1;
}

static int add_first()
{
	B6_LIST_DEFINE(list);
	struct b6_dref dref[2];
	int i;

	for (i = 0; i < b6_card_of(dref); i += 1) {
		if (&dref[i] != b6_list_add_first(&list, &dref[i]))
			return 0;

		if (&dref[i] != b6_list_first(&list))
			return 0;

		if (b6_list_empty(&list))
			return 0;
	}

	return 1;
}

static int add_last()
{
	B6_LIST_DEFINE(list);
	struct b6_dref dref[2];
	int i;

	for (i = 0; i < b6_card_of(dref); i += 1) {
		if (&dref[i] != b6_list_add_last(&list, &dref[i]))
			return 0;

		if (&dref[i] != b6_list_last(&list))
			return 0;

		if (b6_list_empty(&list))
			return 0;
	}

	return 1;
}

static int del_head()
{
	B6_LIST_DEFINE(list);
	int retval;
	jmp_buf env;

	retval = setjmp(env);
	if (!retval) {
		test_handler = &env;
		b6_list_del(b6_list_head(&list));
	}

	return retval;
}

static int del_null()
{
	B6_LIST_DEFINE(list);
	int retval;
	jmp_buf env;

	retval = setjmp(env);
	if (!retval) {
		test_handler = &env;
		b6_list_del(NULL);
	}

	return retval;
}

static int del_tail()
{
	B6_LIST_DEFINE(list);
	int retval;
	jmp_buf env;

	retval = setjmp(env);
	if (!retval) {
		test_handler = &env;
		b6_list_del(b6_list_tail(&list));
	}

	return retval;
}

static int del_first_when_empty()
{
	B6_LIST_DEFINE(list);
	int retval;
	jmp_buf env;

	retval = setjmp(env);
	if (!retval) {
		test_handler = &env;
		b6_list_del_first(&list);
	}

	return retval;
}

static int del_last_when_empty()
{
	B6_LIST_DEFINE(list);
	int retval;
	jmp_buf env;

	retval = setjmp(env);
	if (!retval) {
		test_handler = &env;
		b6_list_del_last(&list);
	}

	return retval;
}

static int del()
{
	B6_LIST_DEFINE(list);
	struct b6_dref dref[3];
	int i;

	for (i = 0; i < b6_card_of(dref); i += 1)
		if (&dref[i] != b6_list_add_last(&list, &dref[i]))
			return 0;

	for (i = 0; i < b6_card_of(dref); i += 1) {
		if (&dref[b6_card_of(dref) - 1] != b6_list_last(&list))
			return 0;

		if (&dref[i] != b6_list_del(&dref[i]))
			return 0;
	}

	if (b6_list_head(&list) != b6_list_last(&list))
		return 0;

	if (!b6_list_empty(&list))
		return 0;

	return 1;
}

/* Not valid anymore since head and tail have merged
static int walk_on_bounds()
{
	B6_LIST_DEFINE(list);

	if (b6_list_walk(&list, b6_list_head(&list), B6_NEXT) !=
	    b6_list_tail(&list))
		return 0;

	if (b6_list_walk(&list, b6_list_head(&list), B6_PREV) != NULL)
		return 0;

	if (b6_list_walk(&list, b6_list_tail(&list), B6_NEXT) != NULL)
		return 0;

	if (b6_list_head(&list) != b6_list_walk(&list, b6_list_tail(&list),
						B6_PREV))
		return 0;

	return 1;
}
*/

static int walk()
{
	B6_LIST_DEFINE(list);
	struct b6_dref dref[16];
	struct b6_dref *iter;
	int i;

	for (i = 0; i < b6_card_of(dref); i += 1)
		if (&dref[i] != b6_list_add_last(&list, &dref[i]))
			return 0;

	for (i = 0, iter = b6_list_first(&list); iter != b6_list_tail(&list);
	     iter = b6_list_walk(iter, B6_NEXT), i += 1)
		if (iter != &dref[i])
			return 0;

	if (i != b6_card_of(dref))
		return 0;

	for (i = 0, iter = b6_list_last(&list); iter != b6_list_head(&list);
	     iter = b6_list_walk(iter, B6_PREV), i += 1)
		if (iter != &dref[b6_card_of(dref) - 1 - i])
			return 0;

	if (i != b6_card_of(dref))
		return 0;

	return 1;
}

/*
 * generate with:
 egrep "^static int.*()" list.c | sed -e 's/(/,/g' -e 's/static int /\ttest(/g' -e 's/$/;/g'
 */

int main(int argc, const char *argv[])
{
	test_init();

	test_exec(always_fails,);
	test_exec(static_init,);
	test_exec(runtime_init,);
	test_exec(last_is_head_when_empty,);
	test_exec(first_is_tail_when_empty,);
	/*test_exec(add_before_head,);*/
	test_exec(add_null,);
	test_exec(add,);
	test_exec(add_first,);
	test_exec(add_last,);
	test_exec(del_head,);
	test_exec(del_null,);
	test_exec(del_tail,);
	test_exec(del_first_when_empty,);
	test_exec(del_last_when_empty,);
	test_exec(del,);
	/*test_exec(walk_on_bounds,);*/
	test_exec(walk,);

	test_exit();

	return 0;
}
