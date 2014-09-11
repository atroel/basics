#include "node.h"

#include "b6/splay.h"

#include <stdio.h>
#include <stdlib.h>

static int splay_cmp(const void *ref1, const void *ref2)
{
	struct node *p1 = b6_container_of(ref1, struct node, dref);
	struct node *p2 = b6_container_of(ref2, struct node, dref);
	return node_cmp(p1, p2);
}

static inline int do_search(struct b6_splay *splay, int *direction,
			    struct b6_dref *dref)
{
	int res;
	res = b6_splay_search(splay, *direction, splay_cmp, dref);
	return res;
}


static inline int do_add(struct b6_splay *splay, struct b6_dref *ref)
{
	int dir, ret;
	if ((ret = do_search(splay, &dir, ref)))
		b6_splay_add(splay, dir, ref);
	return ret;
}

static inline int do_del(struct b6_splay *splay, struct b6_dref *ref)
{
	int dir, ret;
	if ((ret = !do_search(splay, &dir, ref)))
		b6_splay_del(splay);
	return ret;
}

static void do_dbg(struct b6_dref *ref)
{
	printf("%p", ref);
	if (!__b6_splay_is_thread(ref)) {
		struct node *node = b6_cast_of(ref, struct node, dref);
		printf("[%u](", node->val);
		do_dbg(ref->ref[0]);
		printf(",");
		do_dbg(ref->ref[1]);
		printf(")");
	}
}

int main(int argc, const char *argv[])
{
	int retval = 0;
	struct node nodes[16];
	struct b6_splay splay;
	struct b6_dref *ref;
	unsigned u;

	b6_splay_initialize(&splay);

	for (u = b6_card_of(nodes); u--;) {
		struct node *node = &nodes[u];
		node->val = u & 1 ? b6_card_of(nodes) - u : u;
		printf("node[%u] dref=%p, val=%u\n", u, &node->dref, node->val);
		do_add(&splay, &node->dref);
	}

/*	do_dbg(b6_splay_root(&splay)); puts("");*/
	do_del(&splay, &nodes[3].dref);
/*	do_dbg(b6_splay_root(&splay)); puts("");*/

	for (ref = b6_splay_first(&splay); ref != b6_splay_tail(&splay);
	     ref = b6_splay_walk(&splay, ref, B6_NEXT)) {
		struct node *node = b6_cast_of(ref, struct node, dref);
		printf("%u ", node->val);
		fflush(stdout);
	}
	puts("");

	for (ref = b6_splay_last(&splay); ref != b6_splay_head(&splay);
	     ref = b6_splay_walk(&splay, ref, B6_PREV)) {
		struct node *node = b6_cast_of(ref, struct node, dref);
		printf("%u ", node->val);
		fflush(stdout);
	}
	puts("");

	return retval;
}
