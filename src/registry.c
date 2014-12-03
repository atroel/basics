#include "b6/registry.h"

static struct b6_entry *b6_search_registry(struct b6_registry *self,
					   const struct b6_entry *rhs,
					   struct b6_tref **top, int *dir)
{
	struct b6_tref *ref;
	b6_tree_search(&self->tree, ref, *top, *dir) {
		struct b6_entry *lhs = b6_cast_of(ref, struct b6_entry, tref);
		const unsigned char *lname = (unsigned char*)lhs->name;
		const unsigned char *rname = (unsigned char*)rhs->name;
		unsigned short int lsize = lhs->size;
		unsigned short int rsize = rhs->size;
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

static struct b6_entry *setup_ascii_entry(struct b6_entry *entry,
					  const char *name)
{
	unsigned int size;
	unsigned char *p;
	entry->hash = 5381; /* Bernstein hash: hash = hash * 33 + *p */
	for (p = (unsigned char*)name; *p;
	     entry->hash = (entry->hash << 5) + entry->hash + *p++);
	size = p - (unsigned char*)name;
	entry->size = size;
	entry->name = name;
	return size != entry->size ? NULL : entry;
}

static struct b6_entry *setup_utf8_entry(struct b6_entry *entry,
					 const void *name, unsigned int size)
{
	unsigned char *p;
	entry->size = size;
	if (entry->size != size)
		return NULL;
	entry->hash = 5381; /* Bernstein hash: hash = hash * 33 + *p */
	for (p = (unsigned char*)name; size--;
	     entry->hash = (entry->hash << 5) + entry->hash + *p++);
	entry->name = name;
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
		const char *name)
{
	return register_entry(self, setup_ascii_entry(entry, name));
}

int b6_register_utf8(struct b6_registry *self, struct b6_entry *entry,
		     const void *name, unsigned int size)
{
	return register_entry(self, setup_utf8_entry(entry, name, size));
}

struct b6_entry *b6_lookup_registry(struct b6_registry *self, const char *name)
{
	struct b6_entry entry;
	struct b6_tref *top;
	int dir;
	if (!setup_ascii_entry(&entry, name))
		return NULL;
	return b6_search_registry(self, &entry, &top, &dir);
}

struct b6_entry *b6_lookup_registry_utf8(struct b6_registry *self,
					 const void *name, unsigned long size)
{
	struct b6_entry entry;
	struct b6_tref *top;
	int dir;
	if (!setup_utf8_entry(&entry, name, size))
		return NULL;
	return b6_search_registry(self, &entry, &top, &dir);
}
