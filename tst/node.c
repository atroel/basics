#include "node.h"
#include "b6/utils.h"

int node_cmp(const struct node *a, const struct node *b)
{
	return b6_sign_of(a->val - b->val);
}
