#ifndef NODE_H_
#define NODE_H_

#include "b6/refs.h"

struct node {
	struct b6_tref tref;
	struct b6_sref sref;
	struct b6_dref dref;
	int val;
};

int node_cmp(const struct node *a, const struct node *b);

#endif
