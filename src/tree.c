/*
 * Copyright (c) 2009-2015, Arnaud TROEL
 * See LICENSE file for license details.
 */

#include "b6/tree.h"

static inline int get_tag(const struct b6_tref *tref)
{
	return (unsigned long int)tref->top & 3;
}

static inline struct b6_tref *get_top(const struct b6_tref *tref)
{
	return (struct b6_tref *)((unsigned long int)tref->top & ~3);
}

static inline int set_tag(struct b6_tref *tref, int tag)
{
	struct b6_tref *top = get_top(tref);
	tref->top = (struct b6_tref*)((unsigned long int)top | tag);
	return tag;
}

static inline struct b6_tref *set_top(struct b6_tref *tref, struct b6_tref *top)
{
	unsigned long int tag = get_tag(tref);
	tref->top = (struct b6_tref*)((unsigned long int)top | tag);
	return top;
}

static inline void swp_tag_top(struct b6_tref *a, struct b6_tref *b)
{
	struct b6_tref *top = a->top;
	a->top = b->top;
	b->top = top;
}

static void rotate(struct b6_tref *r, int dir, int opp)
{
	struct b6_tref *p = r->ref[opp], *q = p->ref[dir], *t = get_top(r);
	if (p->ref[dir])
		set_top(q, r);
	r->ref[opp] = q;
	set_top(p, t);
	t->ref[t->ref[B6_NEXT] == r ? B6_NEXT : B6_PREV] = p;
	set_top(r, p);
	p->ref[dir] = r;
}

static void insert(struct b6_tref *top, int dir, struct b6_tref *ref)
{
	top->ref[dir] = ref;
	ref->ref[B6_NEXT] = NULL;
	ref->ref[B6_PREV] = NULL;
	ref->top = top;
}

static struct b6_tref *remove(struct b6_tref **top, int *dir)
{
	struct b6_tref *ref, *child, *tmp;
	int direction, opposite;

	ref = (*top)->ref[*dir];

	if (!ref->ref[B6_NEXT]) {
		if ((child = ref->ref[B6_PREV]))
			set_top(child, *top);
		(*top)->ref[*dir] = child;
		return ref;
	}

	if (!ref->ref[B6_PREV]) {
		child = ref->ref[B6_NEXT];
		set_top(child, *top);
		(*top)->ref[*dir] = child;
		return ref;
	}

	direction = !get_tag(ref);
	opposite = b6_to_opposite(direction);

	child = ref->ref[opposite];
	if (!child->ref[direction]) {
		(*top)->ref[*dir] = child;
		swp_tag_top(child, ref);
		child->ref[direction] = ref->ref[direction];
		set_top(child->ref[direction], child);
		*top = child;
		*dir = opposite;
		return ref;
	}

	do {
		tmp = child;
		child = child->ref[direction];
	} while (child->ref[direction]);
	(*top)->ref[*dir] = child;
	swp_tag_top(child, ref);
	tmp->ref[direction] = child->ref[opposite];
	if (tmp->ref[direction])
		set_top(tmp->ref[direction], tmp);
	child->ref[direction] = ref->ref[direction];
	set_top(child->ref[direction], child);
	child->ref[opposite] = ref->ref[opposite];
	set_top(child->ref[opposite], child);
	*top = tmp;
	*dir = direction;
	return ref;
}

static inline int avl_weight(int direction)
{
	return (-(direction) << 1) + 1; /* (direction == B6_PREV ? -1 : 1) */
}

static inline int get_avl_bal(const struct b6_tref *tref)
{
	return get_tag(tref) - 1;
}

static inline int set_avl_bal(struct b6_tref *tref, int bal)
{
	set_tag(tref, bal + 1);
	return bal;
}

/*
 * AVL Tree Rebalancing
 * --------------------
 *
 * Nodes are written lower case, followed by their balance under parentheses.
 * Trees are written upper case, followed by their height under brackets.
 * Note that the height of the whole tree does not change in case SR2 only.
 *
 * A] single rotations (here, right), that is, when balance of p is not right:
 *
 * SR1:     r(-2)              p(0)
 * ----     /   \             /   \
 *      p(-1)   C[h] ==> A[h+1]   r(0)
 *      /   \                     /  \
 * A[h+1]   B[h]               B[h]  C[h]
 *
 * SR2:     r(-2)              p(1)
 * ----     /   \             /   \
 *      p(0)    C[h] ==> A[h+1]   r(-1)
 *      /  \                      /  \
 * A[h+1]  B[h+1]            B[h+1]  C[h]
 *
 *
 * B] double rotations (here, left on p then right on r):
 *
 * DR1:     r(-2)               r                __q(0)__
 * ----     /   \              / \              /         \
 *       p(1)    C[h] ==>     q   C ==>     p(0)          r(0)
 *       /  \                / \           /   \         /   \
 *    A[h] q(0)             p  D        A[h]   B[h]   D[h]   C[h]
 *         /  \            / \
 *      B[h]  D[h]        A   B
 *
 * DR2:     r(-2)               r                __q(0)__
 * ----     /   \              / \              /        \
 *       p(1)    C[h] ==>     q   C ==>     p(-1)         r(0)
 *       /  \                / \           /    \        /   \
 *    A[h] q(1)             p   D       A[h]    B[h-1] D[h]   C[h]
 *         /  \            / \
 *    B[h-1]  D[h]        A   B
 *
 * DR3:     r(-2)               r                __q(0)__
 * ----     /   \              / \              /        \
 *       p(1)    C[h] ==>     q   C ==>     p(0)          r(1)
 *       /  \                / \           /   \          /  \
 *    A[h]  q(-1)           p   D       A[h]   B[h]  D[h-1]  C[h]
 *          /  \           / \
 *       B[h]  D[h-1]     A   B
 */
static int rebalance_avl(struct b6_tref *r, int opp)
{
	struct b6_tref *p = r->ref[opp];
	int change = get_avl_bal(p);
	int dir = b6_to_opposite(opp);
	int weight = avl_weight(dir);

	if (change == weight) {
		struct b6_tref *q = p->ref[dir];
		int bal = get_avl_bal(q);
		set_avl_bal(r, -(((bal - weight) >> 1) & bal));
		set_avl_bal(p, -(((bal + weight) >> 1) & bal));
		b6_assert(get_avl_bal(r) == ((bal == -weight) ? weight : 0));
		b6_assert(get_avl_bal(p) == ((bal == weight) ? -weight : 0));
		set_avl_bal(q, 0);
		rotate(r->ref[opp], opp, dir);
	} else
		set_avl_bal(r, -set_avl_bal(p, change + weight));

	rotate(r, dir, opp);

	return change;
}

/*
 * AVL Tree Insertion
 * ------------------
 *
 * After the node has been inserted, the tree is traveled upwards to adjust
 * possible balances issues. The operation continues until a subtree which
 * height did not change is found. This occur when subtree becomes even after
 * the insertion or when it has to be re-balanced as it will restore its
 * previous height.
 */
static void b6_tree_avl_add(struct b6_tref *top, int dir, struct b6_tref *ref)
{
	insert(top, dir, ref);
	set_avl_bal(ref, 0);

	for (;;) {
		int old_bal, new_bal;

		ref = top;
		top = get_top(ref);
		if (!top)
			break;

		old_bal = get_avl_bal(ref);
		new_bal = old_bal + avl_weight(dir);

		if (!new_bal) {
			set_avl_bal(ref, 0);
			break;
		}

		if (old_bal) {
			new_bal /= 2;
			set_avl_bal(ref, new_bal);
			rebalance_avl(ref, b6_to_direction(new_bal));
			break;
		}

		set_avl_bal(ref, new_bal);
		dir = top->ref[B6_NEXT] == ref ? B6_NEXT : B6_PREV;
	}
}

/*
 * AVL Tree Removal
 * ----------------
 *
 * The tree is traveled upwards to adjust balances until the height of a
 * subtree does not change. This occur either when the subtree was even before
 * the removal or when it had to be re-balanced and that operation did not
 * change its height.
 */
static struct b6_tref *b6_tree_avl_del(struct b6_tref *top, int dir)
{
	struct b6_tref *ref, *ret = remove(&top, &dir);

	for (;;) {
		int old_bal, new_bal;

		ref = top;
		top = get_top(ref);
		if (!top)
			break;

		old_bal = get_avl_bal(ref);
		new_bal = old_bal - avl_weight(dir);

		if (!old_bal) {
			set_avl_bal(ref, new_bal);
			break;
		}

		dir = top->ref[B6_NEXT] == ref ? B6_NEXT : B6_PREV;

		new_bal /= 2;
		set_avl_bal(ref, new_bal);
		if (new_bal && !rebalance_avl(ref, b6_to_direction(new_bal)))
			break;
	}

	return ret;
}

/*
 * AVL Tree Verification
 * ---------------------
 *
 * The tree is traveled in depth first. For each node we check that left
 * height and right height do not differ of more than 1.
 */
static int __b6_tree_avl_chk(struct b6_tref **tref)
{
	int h1, h2;
	struct b6_tref *curr, *prev, *next;

	curr = *tref;
	if ((prev = b6_tree_child(curr, B6_PREV))) {
		*tref = prev;
		h1 = __b6_tree_avl_chk(tref);
		if (h1 < 0)
			return h1;
	} else
		h1 = 0;

	if ((next = b6_tree_child(curr, B6_NEXT))) {
		*tref = next;
		h2 = __b6_tree_avl_chk(tref);
		if (h2 < 0)
			return h2;
	} else
		h2 = 0;

	if (get_avl_bal(curr) + h1 != h2)
		return -1;

	if (h1 > h2)
		if (h1 - h2 > 1)
			return -1;
		else
			return 1 + h1;
	else
		if (h2 - h1 > 1)
			return -1;
		else
			return 1 + h2;
}

int b6_tree_avl_chk(const struct b6_tree *tree, struct b6_tref **tref)
{
	int retval = 0;
	struct b6_tref *root = b6_tree_root(tree);

	if (root)
		retval = __b6_tree_avl_chk(&root);

	if (tref)
		*tref = root;

	return retval;
}

const struct b6_tree_ops b6_tree_avl_ops = {
	.add = b6_tree_avl_add,
	.del = b6_tree_avl_del,
	.chk = b6_tree_avl_chk,
};

static inline void set_black(struct b6_tref *tref)
{
	set_tag(tref, 0);
}

static inline void set_red(struct b6_tref *tref)
{
	set_tag(tref, 1);
}

static inline int is_black(const struct b6_tref *tref)
{
	return !get_tag(tref);
}

static inline int is_red(const struct b6_tref *tref)
{
	return tref && get_tag(tref);
}

/*
 * Red Black Tree Insertion
 * ------------------------
 *
 * This function fixes colors after a node has been inserted in a red-black
 * tree. The recently inserted node is painted red. Then, color are fixed
 * backwards so as not break red-black tree rules. The loop begins at step #2.
 *
 * 1] If the node is the root of the tree, it is colored black and the
 * operation is over as it adds a black node to every path of the tree.
 *
 * 2] The operation is over as well if the parent of the node is black as
 * rules remain satisfied.
 *
 * 3] The node as a red parent, which induces that it also has a grandparent
 * as the root of tree is black. If the node has a red uncle, then the parent
 * and the uncle are turned black while the grandparent is turned red. This
 * does not violate the 4th rule. The operation has to go on with the
 * grandparent as it could still break the other rules.
 *
 * 4] Now, either the node has no uncle or its uncle is painted black, like a
 * Rolling Stone. If the parent is a left (resp. right) child and the node a
 * right (resp. left), then a left (resp. right) rotation will invert their
 * roles, as follows:
 *
 *        _g[b]__                         __g[b]_
 *       /       \                       /       \
 *    p[r]        u[b]     ==>        n[r]        u[b]
 *    /  \        /  \                /  \        /  \
 * A[b]  n[r]  D[?]  E[?]          p[r]  C[b]  D[?]  E[?]
 *       /  \                      /  \
 *    B[b]  C[b]                A[b]  B[b]
 *
 * The rotation is safe as both the node and its parent are red and properties
 * are not violated in any sub-tree.
 *
 * 5] The former parent has to be dealt with, however, as it breaks the 3rd
 * rule. Now, we know that parent and node are either both left (resp. right)
 * children. Thus, a right (resp. left) rotation rooted at the child of the
 * grand parent (e.g. the parent or the former node according to the previous
 * configuration) will make the former grandparent a child of the parent.
 * Then, switching colors of both nodes will restore the rules and the
 * operation is complete.
 *
 *           __g[b]_                      _p[r->b]_
 *          /       \                    /         \
 *       p[r]        u[b]             n[r]          g[b->r]
 *       /  \        /  \             /  \          /     \
 *    n[r]  C[b]  D[?]  E[?]  ==>  A[b]  B[b]    C[b]     u[b]
 *    /  \                                                /  \
 * A[b]  B[b]                                          D[?]  E[?]
 */
static void b6_tree_rb_add(struct b6_tref *top, int dir, struct b6_tref *ref)
{
	struct b6_tref *elder = get_top(top);

	insert(top, dir, ref);

	if (!elder) {
		set_black(ref);
		return;
	}

	set_red(ref);

	while (!is_black(top)) {
		int direction = elder->ref[B6_NEXT] == top ? B6_NEXT : B6_PREV;
		int opposite = b6_to_opposite(direction);
		struct b6_tref *uncle = elder->ref[opposite];

		if (is_red(uncle)) {
			set_black(top);
			set_black(uncle);
			set_red(elder);
			ref = elder;
			top = get_top(ref);
			elder = get_top(top);

			if (elder)
				continue;

			set_black(ref);
			break;
		}

		if (top->ref[direction] != ref) {
			rotate(top, direction, opposite);
			uncle = top;
			top = ref;
			ref = uncle;
		}
		set_black(top);
		set_red(elder);

		rotate(elder, opposite, direction);
		break;
	}
}

/*
 * Red Black Tree Removal
 * ----------------------
 *
 * This function fixes colors after a node has been removed from a red-black
 * tree.
 *
 * If the removed node was red, then we are done as no black path has changed
 * at all.
 *
 * Now, we know the removed node was black. It was replaced by one of its
 * children. If this child is red, then, we repaint it black to restore black
 * height.
 *
 * Otherwise, the child is black. If its sibling is red, it is possible to
 * rotate the sub-tree so that it becomes the parent of the parent. Colors
 * have to be exchanged between both of them to avoid breaking rules once
 * more. This insures the sibling is black for the remainder of the
 * discussion. Here, we based the algorithm that it can be proven that a
 * sibling always exists in that case.
 *
 * If both sibling's children are black (or if the sibling is a leaf), then
 * repainting the sibling red will make all paths across it contain the same
 * number of black node as those passing through the node. Now, the parent
 * itself has to be examined as it still breaks the rules since all paths
 * through it have one fewer black node. If it is red, then we repaint it
 * black to add the missing black node in the path and we are done.
 *
 * If sibling's children have different colors, we insure that the sibling has
 * a red child in the same direction as the node. If not, a rotation in the
 * opposite direction rooted at sibling will do it. Colors are exchanged
 * between the former and the new sibling. As a result, number of black nodes
 * are kept unchanged.
 *
 * Now, a rotation rooted at the parent in the direction of the child will
 * make the sibling the parent of the former parent. The new parent gets the
 * former parent's color. Painting its children black will restore red/black
 * properties. Paths passing through the removed node have one additional
 * black node. Other paths either pass through its sibling or its uncle. The
 * first ones are ok since parent and sibling colors have not changed. Thoses
 * passing through the new uncle, have got one new black node as one node was
 * changed from red to black, balancing the node they lost during the
 * rotation.
 */
static struct b6_tref *b6_tree_rb_del(struct b6_tref *top, int dir)
{
	struct b6_tref *ret = remove(&top, &dir);

	if (!is_black(ret))
		return ret;

	if (is_red(top->ref[dir])) {
		set_black(top->ref[dir]);
		return ret;
	}

	while (get_top(top)) {
		struct b6_tref *elder;
		int opp_is_red;
		int opp = b6_to_opposite(dir);
		struct b6_tref *sibling = top->ref[opp];

		b6_assert(sibling);

		if (!is_black(sibling)) {
			set_red(top);
			set_black(sibling);
			rotate(top, dir, opp);
			sibling = top->ref[opp];
			b6_assert(!is_red(sibling));
		}

		opp_is_red = is_red(sibling->ref[opp]);
		if (opp_is_red || is_red(sibling->ref[dir])) {
			if (!opp_is_red) {
				set_black(sibling->ref[dir]);
				set_red(sibling);
				rotate(sibling, opp, dir);
				sibling = top->ref[opp];
				b6_assert(sibling);
			}
			set_tag(sibling, get_tag(top));
			set_black(top);
			set_black(sibling->ref[opp]);
			rotate(top, dir, opp);
			break;
		}

		set_red(sibling);
		if (!is_black(top)) {
			set_black(top);
			break;
		}
		elder = get_top(top);
		dir = elder->ref[B6_NEXT] == top ? B6_NEXT : B6_PREV;
		top = elder;
	}

	return ret;
}

/*
 * Red-Black Tree Verification
 * ---------------------------
 *
 * The tree is traveled in depth first to verify the following rules:
 *
 * 1] A node is either red or black.
 * 2] The root is black.
 * 3] Both children of every red node are black.
 * 4] Every simple path from a node to a descendant leaf contains the same
 *    number of black nodes.
 * 5] Leaves are all considered black nodes.
 */
static int __b6_tree_rb_chk(struct b6_tref **tref)
{
	struct b6_tref *prev, *next, *curr;
	int h;

	curr = *tref;
	if ((prev = b6_tree_child(curr, B6_PREV))) {
		*tref = prev;
		h = __b6_tree_rb_chk(tref);
		if (h < 0)
			return h;

		if ((next = b6_tree_child(curr, B6_NEXT))) {
			int tmp;
			*tref = next;
			tmp = __b6_tree_rb_chk(tref);
			if (tmp < 0)
				return tmp;

			if (tmp != h)
				return -1;
		}
	} else if ((next = b6_tree_child(curr, B6_NEXT))) {
		*tref = next;
		h = __b6_tree_rb_chk(tref);
		if (h < 0)
			return h;
	} else
		h = 0;

	if (is_black(curr))
		return 1 + h;
	else if (!is_red(prev) && !is_red(next))
		return h;
	else {
		*tref = curr;
		return -2;
	}
}

static int b6_tree_rb_chk(const struct b6_tree *tree, struct b6_tref **tref)
{
	struct b6_tref *root = b6_tree_root(tree);
	int retval = is_red(root) ? -2 :  __b6_tree_rb_chk(&root);

	if (tref)
		*tref = root;

	return retval;
}

const struct b6_tree_ops b6_tree_rb_ops = {
	.add = b6_tree_rb_add,
	.del = b6_tree_rb_del,
	.chk = b6_tree_rb_chk,
};

/*
 * AVL/Red-Black Tree Traveling
 * ----------------------------
 *
 * If the node has a child in the traveling direction, the node to find It is
 * the latest node in the opposite direction of the subtree of the
 * child. Otherwise, the next node is the first elder when walking the tree
 * backwards to the root, which child is in the opposite direction.
 */
struct b6_tref *__b6_tree_dive(const struct b6_tref *ref, int dir)
{
	struct b6_tref *child;

	while ((child = b6_tree_child(ref, dir)))
		ref = child;

	return (struct b6_tref *)ref;
}

struct b6_tref *__b6_tree_walk(const struct b6_tref *ref, int dir)
{
	struct b6_tref *top;

	while ((top = get_top(ref))) {
		int opp = top->ref[dir] != ref;
		ref = top;
		if (opp)
			break;
	}

	return (struct b6_tref *)ref;
}
