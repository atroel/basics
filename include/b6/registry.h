/*
 * Copyright (c) 2014, Arnaud TROEL
 * See LICENSE file for license details.
 */

/**
 * @file registry.h
 * @brief Store and retrieve objects by name.
 */

#ifndef B6_REGISTRY_H
#define B6_REGISTRY_H

#include "b6/tree.h"

/**
 * @brief A registry stores named entries and allows quick lookups by name.
 *
 * Registries can be populated and searched like this:
 *
 * @code
 * struct example {
 *   struct b6_entry entry;
 *   int some_value;
 *   float another_value;
 * };
 *
 * B6_REGISTRY_DEFINE(example_registry);
 *
 * int register_example(struct example *example, const char *name)
 * {
 *   return b6_register(&example->entry, name);
 * }
 *
 * struct example *lookup_example_by_name(const char *name)
 * {
 *   struct b6_entry *e = b6_lookup_registry(name);
 *   return entry ? b6_cast_of(e, struct example, entry) : NULL;
 * }
 * @endcode
 *
 * A registry can be traveled as follows:
 *
 * @code
 * struct example {
 *   struct b6_entry entry;
 *   int some_value;
 *   float another_value;
 * };
 *
 * B6_DEFINE_REGISTRY(example_registry);
 *
 * void print_examples(void)
 * {
 *   struct b6_entry *entry = b6_get_first_entry(&example_registry);
 *   while ((entry = b6_get_next_entry(&example_registry, entry))) {
 *     struct example *ex = b6_cast_of(entry, struct example, entry);
 *     printf("s=%d a=%f\n", ex->some_value, ex->another_value);
 *   }
 * }
 * @endcode
 */
struct b6_registry {
	struct b6_tree tree;
};

/**
 * @brief Members of a registry.
 */
struct b6_entry {
	struct b6_tref tref;
	unsigned int hash;
	const char *name;
};

/**
 * @brief Define a registry.
 */
#define B6_REGISTRY_DEFINE(registry) \
	struct b6_registry registry = { B6_TREE_INIT(&b6_tree_rb_ops), }

/**
 * @brief Initialize a registry.
 * @param self specifies the registry to setup.
 */
static inline void b6_setup_registry(struct b6_registry *self)
{
	b6_tree_initialize(&self->tree, &b6_tree_rb_ops);
}

/**
 * @internal
 */
extern struct b6_entry *b6_search_registry(struct b6_registry*, unsigned int,
					   const char*, struct b6_tref**, int*);

/**
 * @internal
 */
extern unsigned int b6_compute_registry_hash(const char*);

/**
 * @brief Add an entry to a registry.
 * @param self specifies the registry to populate.
 * @param entry specifies the entry to add.
 * @param name specifies the name of the entry.
 */
static inline int b6_register(struct b6_registry *self, struct b6_entry *entry,
			      const char *name)
{
	struct b6_tref *top;
	int dir;
	entry->hash = b6_compute_registry_hash(name);
	entry->name = name;
	if (b6_search_registry(self, entry->hash, entry->name, &top, &dir))
		return -1;
	b6_tree_add(&self->tree, top, dir, &entry->tref);
	return 0;
}

/**
 * @brief Remove an entry from a registry.
 * @pre The entry to remove must be a member of the registry.
 * @param self specifies the registry.
 * @param entry specifies the entry to remove.
 */
static inline void b6_unregister(struct b6_registry *self,
				 struct b6_entry *entry)
{
	int dir;
	struct b6_tref *top = b6_tree_parent(&entry->tref, &dir);
	b6_tree_del(&self->tree, top, dir);
}

/**
 * @brief Find a registry entry by name.
 * @param self specifies the registry to search.
 * @param name specifies the name of the entry to find.
 * @return a pointer to the entry.
 * @return NULL if no entry with such a name was found.
 */
static inline struct b6_entry *b6_lookup_registry(struct b6_registry *self,
						  const char *name)
{
	struct b6_tref *top;
	int dir;
	return b6_search_registry(self, b6_compute_registry_hash(name), name,
				  &top, &dir);
}

/**
 * @brief Get the first entry of a registry in the sequential order.
 * @param r specifies the registry
 * @return a pointer the the entry.
 * @return NULL is the registry is empty.
 */
static inline struct b6_entry *b6_get_first_entry(const struct b6_registry *r)
{
	struct b6_tref *tref = b6_tree_first(&r->tree);
	if (tref == b6_tree_tail(&r->tree))
	       return NULL;
	return b6_cast_of(tref, struct b6_entry, tref);
}

/**
 * @brief Get the last entry of a registry in the sequential order.
 * @param r specifies the registry
 * @return a pointer the the entry.
 * @return NULL is the registry is empty.
 */
static inline struct b6_entry *b6_get_last_entry(const struct b6_registry *r)
{
	struct b6_tref *tref = b6_tree_last(&r->tree);
	if (tref == b6_tree_head(&r->tree))
	       return NULL;
	return b6_cast_of(tref, struct b6_entry, tref);
}

/**
 * @brief Get the next entry of another in the sequential order.
 * @param r specifies the registry.
 * @param e specifies an entry of the registry.
 * @return a pointer to the next entry.
 * @return NULL if the end of the registry has been reached.
 */
static inline struct b6_entry *b6_walk_registry(const struct b6_registry *r,
						const struct b6_entry *e,
						int direction)
{
	struct b6_tref *tref = b6_tree_walk(&r->tree, &e->tref, direction);
	if ((direction == B6_NEXT && tref == b6_tree_tail(&r->tree)) ||
	    (direction == B6_PREV && tref == b6_tree_head(&r->tree)))
	       return NULL;
	return b6_cast_of(tref, struct b6_entry, tref);
}

#endif /* B6_REGISTRY_H */
