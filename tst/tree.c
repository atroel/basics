#include "test.h"

#include "b6/list.h"
#include "b6/tree.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

struct node {
	struct b6_tref tref;
	struct b6_dref dref;
};

static int do_cmp(const struct node *self, const struct node *node)
{
	return b6_sign_of((unsigned long int)self - (unsigned long int)node);
}

static struct b6_tref *do_add(struct b6_tree *tree, struct b6_tref *tref)
{
	struct b6_tref *top, *ref;
	int dir;
	struct node *n1 = b6_cast_of(tref, struct node, tref);

	b6_tree_search(tree, ref, top, dir) {
		struct node *n2 = b6_cast_of(ref, struct node, tref);
		int result = do_cmp(n2, n1);

		if (b6_unlikely(!result))
			return ref;

		dir = b6_to_direction(result);
	}

	return b6_tree_add(tree, top, dir, tref);
}

static struct b6_tref *do_del(struct b6_tree *tree, struct b6_tref *tref)
{
	int dir;
	struct b6_tref *top = b6_tree_parent(tref, &dir);
	return b6_tree_del(tree, top, dir);
}

static int always_fails(void)
{
	return 0;
}

static int first_is_tail_when_empty(void)
{
	struct b6_tree tree;
	struct b6_tref *tref;

	b6_tree_initialize(&tree, NULL);
	tref = b6_tree_first(&tree);
	return tref == b6_tree_tail(&tree);
}

static int last_is_head_when_empty(void)
{
	struct b6_tree tree;
	struct b6_tref *tref;

	b6_tree_initialize(&tree, NULL);
	tref = b6_tree_last(&tree);
	return tref == b6_tree_head(&tree);
}

static int first_is_smallest(void)
{
	struct node *n, nodes[4];
	struct b6_tree tree;
	unsigned int u;

	b6_tree_initialize(&tree, &b6_tree_avl_ops);

	for (u = b6_card_of(nodes), n = &nodes[0]; u--;
	     do_add(&tree, &(n++)->tref));

	return b6_tree_first(&tree) == &nodes[0].tref;
}

static int last_is_greatest(void)
{
	struct node *n, nodes[4];
	struct b6_tree tree;
	unsigned int u;

	b6_tree_initialize(&tree, &b6_tree_avl_ops);

	for (u = b6_card_of(nodes), n = &nodes[0]; u--;
	     do_add(&tree, &(n++)->tref));

	return b6_tree_last(&tree) == &nodes[b6_card_of(nodes) - 1].tref;
}

static int walk_next(void)
{
	struct node *n, nodes[4];
	struct b6_tree tree;
	struct b6_tref *tref;
	unsigned int u;
	int retval = 0;

	b6_tree_initialize(&tree, &b6_tree_avl_ops);

	for (u = b6_card_of(nodes), n = &nodes[0]; u--;
	     do_add(&tree, &(n++)->tref));

	for (u = 0, tref = b6_tree_first(&tree); u < b6_card_of(nodes); u++) {
		retval = (tref != b6_tree_tail(&tree));
		if (!retval)
			goto bail_out;
		retval = (tref == &nodes[u].tref);
		if (!retval)
			goto bail_out;
		tref = b6_tree_walk(&tree, tref, B6_NEXT);
	}
	retval = (tref == b6_tree_tail(&tree));

bail_out:
	return retval;
}

static int walk_prev(void)
{
	struct node *n, nodes[4];
	struct b6_tree tree;
	struct b6_tref *tref;
	unsigned int u;
	int retval = 0;

	b6_tree_initialize(&tree, &b6_tree_avl_ops);

	for (u = b6_card_of(nodes), n = &nodes[0]; u--;
	     do_add(&tree, &(n++)->tref));

	for (u = b6_card_of(nodes), tref = b6_tree_last(&tree);
	     tref != b6_tree_head(&tree);
	     tref = b6_tree_walk(&tree, tref, B6_PREV))
		if (!(retval = (tref == &nodes[--u].tref)))
			break;


	for (u = b6_card_of(nodes), tref = b6_tree_last(&tree); u--; ) {
		retval = (tref != b6_tree_head(&tree));
		if (!retval)
			goto bail_out;
		retval = (tref == &nodes[u].tref);
		if (!retval)
			goto bail_out;
		tref = b6_tree_walk(&tree, tref, B6_PREV);
	}
	retval = (tref == b6_tree_head(&tree));

bail_out:
	return retval;
}

static int thread_should_exit = 0;

static void *endurance_thread(void *arg)
{
	struct node *n, nodes[256];
	struct b6_tree tree;
	struct b6_list list;
	struct b6_tref *dbg;
	unsigned int u, ilen, olen;
	int retval;

	b6_tree_initialize(&tree, arg);
	b6_list_initialize(&list);

	for (u = b6_card_of(nodes), n = &nodes[u]; u--; )
		b6_list_add_last(&list, &(--n)->dref);
	olen = b6_card_of(nodes);
	ilen = 0;

	while (!thread_should_exit) {
		for (u = olen ? rand() % olen : 0; u--; olen -= 1, ilen += 1) {
			struct b6_dref *dref = b6_list_head(&list);
			int dir = rand() & 1;
			unsigned int v = rand() % olen;
			do dref = b6_list_walk(dref, dir); while (v--);
			b6_list_del(dref);
			n = b6_cast_of(dref, struct node, dref);
			do_add(&tree, &n->tref);
			if (0 > (retval = b6_tree_check(&tree, &dbg)))
				goto bail_out;
		}

		for (u = ilen ? rand() % ilen : 0; u--; ilen -= 1, olen += 1) {
			struct b6_tref *tref = b6_tree_head(&tree);
			int dir = rand() & 1;
			unsigned int v = rand() % ilen;
			do tref = b6_tree_walk(&tree, tref, dir); while (v--);
			do_del(&tree, tref);
			if (0 > (retval = b6_tree_check(&tree, &dbg)))
				goto bail_out;
			n = b6_cast_of(tref, struct node, tref);
			b6_list_add_last(&list, &n->dref);
		}
	}

	return (void *)1;
bail_out:
	return (void *)0;
}

static int endurance()
{
	void *args[] = { (void *)&b6_tree_avl_ops, (void *)&b6_tree_rb_ops };
	pthread_t threads[b6_card_of(args)];
	void *retvals[b6_card_of(args)];
	int i, retval;

	for (i = b6_card_of(threads); i--;
	     pthread_create(&threads[i], NULL, endurance_thread, args[i]));

	sleep(5 * 60);
	thread_should_exit = 1;

	for (i = b6_card_of(threads); i--;
	     pthread_join(threads[i], &retvals[i]));

	for (retval = 1, i = b6_card_of(threads); i--;
	     retval &= (int)retvals[i]);

	return retval;
}

int main(int argc, const char *argv[])
{
	test_init();
	test_exec(always_fails,);
	test_exec(first_is_tail_when_empty,);
	test_exec(last_is_head_when_empty,);
	test_exec(first_is_smallest,);
	test_exec(last_is_greatest,);
	test_exec(walk_next,);
	test_exec(walk_prev,);
	test_exec(endurance,);
	test_exit();

	return 0;
}
