#include "b6/registry.h"

static struct b6_entry *b6_search_registry(struct b6_registry *self,
					   const struct b6_entry *rhs,
					   struct b6_tref **top, int *dir)
{
	struct b6_tref *ref;
	b6_tree_search(&self->tree, ref, *top, *dir) {
		struct b6_entry *lhs = b6_cast_of(ref, struct b6_entry, tref);
		const unsigned char *lname = (unsigned char*)lhs->id->ptr;
		const unsigned char *rname = (unsigned char*)rhs->id->ptr;
		unsigned short int lsize = lhs->id->nbytes;
		unsigned short int rsize = rhs->id->nbytes;
		if (lhs->hash < rhs->hash) {
			*dir = B6_PREV;
			continue;
		}
		*dir = B6_NEXT;
		if (lhs->hash > rhs->hash)
			continue;
		for (;;) {
			unsigned char l = *lname++;
			unsigned char r = *rname++;
			if (!lsize--) {
				if (!rsize)
					return lhs;
				*dir = B6_PREV;
				break;
			}
			if (!rsize--)
				break;
			if (l == r)
				continue;
			if (l < r)
				*dir = B6_PREV;
			break;
		}
	}
	return NULL;
}

static struct b6_entry *setup_entry(struct b6_entry *entry,
				    const struct b6_utf8 *id)
{
	unsigned int nbytes = id->nbytes;
	unsigned char *ptr = (unsigned char*)id->ptr;
	entry->hash = 5381; /* Bernstein hash: hash = hash * 33 + *ptr */
	while (nbytes--)
	     entry->hash = (entry->hash << 5) + entry->hash + *ptr++;
	entry->id = id;
	return entry;
}

static int register_entry(struct b6_registry *self, struct b6_entry *entry)
{
	struct b6_tref *top;
	int dir;
	if (!entry)
		return -2;
	if (b6_search_registry(self, entry, &top, &dir))
		return -1;
	b6_tree_add(&self->tree, top, dir, &entry->tref);
	return 0;
}

int b6_register(struct b6_registry *self, struct b6_entry *entry,
		const struct b6_utf8 *id)
{
	return register_entry(self, setup_entry(entry, id));
}

struct b6_entry *b6_lookup_registry(struct b6_registry *self,
				    const struct b6_utf8 *name)
{
	struct b6_entry entry;
	struct b6_tref *top;
	int dir;
	if (!setup_entry(&entry, name))
		return NULL;
	return b6_search_registry(self, &entry, &top, &dir);
}
