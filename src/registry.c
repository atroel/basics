#include "b6/registry.h"

/* Bernstein hash */
unsigned int b6_compute_registry_hash(const char *s)
{
	unsigned int hash = 5381;
	while (*s)
		hash = (hash << 5) + hash + *s++; /* hash = hash * 33 + *s */
	return hash;
}

/* string compare */
static int b6_compare_registry_names(const char *lhs, const char *rhs)
{
	for (;;) {
		char l = *lhs++;
		char r = *rhs++;
		if (l < r)
			return -1;
		if (l > r)
			return 1;
		if (!l)
			return 0;
	}
}

struct b6_entry *b6_search_registry(struct b6_registry *self,
				    unsigned int hash, const char *name,
				    struct b6_tref **top, int *dir)
{
	struct b6_tref *ref;
	b6_tree_search(&self->tree, ref, *top, *dir) {
		struct b6_entry *entry = b6_cast_of(ref, struct b6_entry, tref);
		if (entry->hash < hash)
			*dir = B6_PREV;
		else if (entry->hash > hash)
			*dir = B6_NEXT;
		else {
			int cmp = b6_compare_registry_names(entry->name, name);
			*dir = cmp > 0 ? B6_PREV : B6_NEXT;
			if (!cmp)
				return entry;
		}
	}
	return NULL;
}
