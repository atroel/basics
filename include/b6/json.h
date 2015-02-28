#ifndef B6_JSON_H
#define B6_JSON_H

#include "b6/allocator.h"
#include "b6/array.h"
#include "b6/pool.h"
#include "b6/tree.h"

enum b6_json_error {
	B6_JSON_OK = 0,
	B6_JSON_ERROR = -1,
	B6_JSON_IO_ERROR = -2,
	B6_JSON_ALLOC_ERROR = -3,
	B6_JSON_PARSE_ERROR = -4,
};

struct b6_json_istream {
	const struct b6_json_istream_ops *ops;
	char c;
	short int has_c;
};

struct b6_json_istream_ops {
	long int (*read)(struct b6_json_istream*, void*, unsigned long int);
};

static inline void b6_json_setup_istream(struct b6_json_istream *self,
					 const struct b6_json_istream_ops *ops)
{
	self->ops = ops;
	self->has_c = 0;
}

struct b6_json_ostream {
	const struct b6_json_ostream_ops *ops;
};

static inline void b6_json_setup_ostream(struct b6_json_ostream *self,
					 const struct b6_json_ostream_ops *ops)
{
	self->ops = ops;
}


struct b6_json_ostream_ops {
	long int (*write)(struct b6_json_ostream*, const void*,
			  unsigned long int);
	int (*flush)(struct b6_json_ostream*);
};

struct b6_json_value {
	const struct b6_json_value_ops *ops;
	unsigned int refcount;
};

struct b6_json_serializer;

struct b6_json_value_ops {
	void (*dtor)(struct b6_json_value*);
	int (*serialize)(const struct b6_json_value*, struct b6_json_ostream*,
			 struct b6_json_serializer*);
};

struct b6_json_null {
	struct b6_json_value up;
};

struct b6_json_true {
	struct b6_json_value up;
};

struct b6_json_false {
	struct b6_json_value up;
};

struct b6_json_number {
	struct b6_json_value up;
	double number;
	struct b6_json *json;
};

struct b6_json_string {
	struct b6_json_value up;
	struct b6_json_string_impl *impl;
	struct b6_json *json;
};

struct b6_json_array {
	struct b6_json_value up;
	struct b6_json_array_impl *impl;
	struct b6_json *json;
};

struct b6_json_object {
	struct b6_json_value up;
	struct b6_json_object_impl *impl;
	struct b6_json *json;
};

struct b6_json_pair {
	struct b6_json_string *key;
	struct b6_json_value *value;
};

struct b6_json_iterator {
	const struct b6_json_object *object;
	struct b6_json_pair *pair;
};

struct b6_json {
	struct b6_pool pool;
	struct b6_json_impl *impl;
	struct b6_json_null json_null;
	struct b6_json_true json_true;
	struct b6_json_false json_false;
};

struct b6_json_impl {
	const struct b6_json_impl_ops *ops;
};

struct b6_json_impl_ops {
	struct b6_json_array_impl *(*array_impl)(struct b6_json_impl*);
	struct b6_json_string_impl *(*string_impl)(struct b6_json_impl*);
	struct b6_json_object_impl *(*object_impl)(struct b6_json_impl*);
};

struct b6_json_parser_info {
	unsigned int row;
	unsigned int col;
};

static inline void b6_json_reset_parser_info(struct b6_json_parser_info *self)
{
	self->row = 1;
	self->col = 0;
}

struct b6_json_serializer {
	const struct b6_json_serializer_ops *ops;
};

struct b6_json_serializer_ops {
	enum b6_json_error (*enter_object)(struct b6_json_serializer*,
					   struct b6_json_ostream*,
					   const struct b6_json_object*);
	enum b6_json_error (*leave_object)(struct b6_json_serializer*,
					   struct b6_json_ostream*,
					   const struct b6_json_object*);
	enum b6_json_error (*enter_object_key)(struct b6_json_serializer*,
					       struct b6_json_ostream*,
					       const struct b6_json_string*);
	enum b6_json_error (*leave_object_key)(struct b6_json_serializer*,
					       struct b6_json_ostream*,
					       const struct b6_json_string*);
	enum b6_json_error (*enter_object_value)(struct b6_json_serializer*,
						 struct b6_json_ostream*,
						 const struct b6_json_value*);
	enum b6_json_error (*leave_object_value)(struct b6_json_serializer*,
						 struct b6_json_ostream*,
						 const struct b6_json_value*,
						 int);
	enum b6_json_error (*enter_array)(struct b6_json_serializer*,
					  struct b6_json_ostream*,
					  const struct b6_json_array*);
	enum b6_json_error (*leave_array)(struct b6_json_serializer*,
					  struct b6_json_ostream*,
					  const struct b6_json_array*);
	enum b6_json_error (*enter_array_value)(struct b6_json_serializer*,
						struct b6_json_ostream*,
						const struct b6_json_value*);
	enum b6_json_error (*leave_array_value)(struct b6_json_serializer*,
						struct b6_json_ostream*,
						const struct b6_json_value*,
						int);
};

static inline void b6_json_setup_value(struct b6_json_value *self,
				       const struct b6_json_value_ops *ops)
{
	self->ops = ops;
	self->refcount = 1;
}

static inline struct b6_json_value *b6_json_ref_value(struct b6_json_value *s)
{
	s->refcount += 1;
	return s;
}

static inline void b6_json_unref_value(struct b6_json_value *self)
{
	self->refcount -= 1;
	if (!self->refcount && self->ops->dtor)
		self->ops->dtor(self);
}

static inline enum b6_json_error b6_json_serialize_value(
	const struct b6_json_value *self, struct b6_json_ostream *os,
	struct b6_json_serializer *serializer)
{
	enum b6_json_error error = self->ops->serialize(self, os, serializer);
	if (!error)
		os->ops->flush(os);
	return error;
}

#define b6_json_value_as(self, type) b6_cast_of(self, struct b6_json_##type, up)

#define b6_json_value_is_of(self, type) ((self)->ops == &b6_json_##type##_ops)

#define b6_json_value_as_or_null(self, type) \
({ \
	struct b6_json_value *_self = (self); \
	_self && b6_json_value_is_of(_self, type) ? \
		b6_json_value_as(_self, type) : NULL; \
})

const struct b6_json_value_ops b6_json_object_ops;
const struct b6_json_value_ops b6_json_number_ops;
const struct b6_json_value_ops b6_json_string_ops;
const struct b6_json_value_ops b6_json_array_ops;
const struct b6_json_value_ops b6_json_false_ops;
const struct b6_json_value_ops b6_json_true_ops;
const struct b6_json_value_ops b6_json_null_ops;

static inline void b6_json_initialize(struct b6_json *self,
				      struct b6_json_impl *impl,
				      struct b6_allocator *allocator)
{
	union b6_json_element {
		struct b6_json_string s;
		struct b6_json_object o;
		struct b6_json_number n;
		struct b6_json_array a;
	};
	b6_json_setup_value(&self->json_null.up, &b6_json_null_ops);
	b6_json_setup_value(&self->json_true.up, &b6_json_true_ops);
	b6_json_setup_value(&self->json_false.up, &b6_json_false_ops);
	self->impl = impl;
	b6_pool_initialize(&self->pool, allocator,
			   sizeof(union b6_json_element), 0);
}

static inline void b6_json_finalize(struct b6_json *self)
{
	b6_pool_finalize(&self->pool);
}

extern const struct b6_json_serializer_ops b6_json_default_serializer_ops;

struct b6_json_default_serializer {
	struct b6_json_serializer up;
	unsigned int depth;
};

static inline void b6_json_setup_default_serializer(
	struct b6_json_default_serializer *self)
{
	self->up.ops = &b6_json_default_serializer_ops;
	self->depth = 0;
}

static inline struct b6_json_null *b6_json_new_null(struct b6_json *self)
{
	return &self->json_null;
}

static inline struct b6_json_true *b6_json_new_true(struct b6_json *self)
{
	return &self->json_true;
}

static inline struct b6_json_false *b6_json_new_false(struct b6_json *self)
{
	return &self->json_false;
}

static inline void b6_json_set_number(struct b6_json_number *self,
				      double number)
{
	self->number = number;
}

static inline double b6_json_get_number(const struct b6_json_number *self)
{
	return self->number;
}

static inline struct b6_json_number *b6_json_new_number(struct b6_json *json,
							double d)
{
	struct b6_json_number *self;
	if (!(self = b6_pool_get(&json->pool)))
		return NULL;
	b6_json_setup_value(&self->up, &b6_json_number_ops);
	self->number = d;
	self->json = json;
	return self;
}

struct b6_json_string_impl {
	const struct b6_json_string_impl_ops *ops;
};

struct b6_json_string_impl_ops {
	void (*dtor)(struct b6_json_string_impl*, struct b6_json_impl*);
	unsigned int (*size)(const struct b6_json_string_impl*);
	const void *(*utf8)(const struct b6_json_string_impl*);
	enum b6_json_error (*append)(struct b6_json_string_impl*,
				     struct b6_json_impl*,
				     const void*, unsigned int);
};

static inline unsigned int b6_json_string_size
(const struct b6_json_string *self)
{
	return self->impl->ops->size(self->impl);
}

static inline const void *b6_json_string_utf8(const struct b6_json_string *self)
{
	return self->impl->ops->utf8(self->impl);
}

static inline struct b6_json_string *b6_json_new_string(struct b6_json *json,
							const char *str);

static inline enum b6_json_error b6_json_set_string(struct b6_json_string *self,
						    const char *utf8,
						    unsigned int size)
{
	struct b6_json_string *temp;
	enum b6_json_error error;
	if (!size)
		return B6_JSON_OK;
	if (!(temp = b6_json_new_string(self->json, NULL)))
		return B6_JSON_ALLOC_ERROR;
	if (!(error = temp->impl->ops->append(temp->impl, temp->json->impl,
					      utf8, size))) {
		struct b6_json_string_impl *impl = self->impl;
		self->impl = temp->impl;
		temp->impl = impl;
	}
	b6_json_unref_value(&temp->up);
	return B6_JSON_OK;
}

static inline struct b6_json_string *b6_json_new_string(struct b6_json *json,
							const char *str)
{
	struct b6_json_string *self;
	if (!(self = b6_pool_get(&json->pool)))
		return NULL;
	if (!(self->impl = json->impl->ops->string_impl(json->impl))) {
		b6_pool_put(&json->pool, self);
		return NULL;
	}
	b6_json_setup_value(&self->up, &b6_json_string_ops);
	self->json = json;
	if (str) {
		const char *end;
		for (end = str; *end; end += 1);
		if (b6_json_set_string(self, str, end - str)) {
			b6_json_unref_value(&self->up);
			return NULL;
		}
	}
	return self;
}

struct b6_json_array_impl {
	const struct b6_json_array_impl_ops *ops;
};

struct b6_json_array_impl_ops {
	void (*dtor)(struct b6_json_array_impl*, struct b6_json_impl*);
	unsigned int (*len)(const struct b6_json_array_impl*);
	enum b6_json_error (*add)(struct b6_json_array_impl*, unsigned int,
				  struct b6_json_value*);
	void (*del)(struct b6_json_array_impl*, unsigned int);
	struct b6_json_value* (*get)(const struct b6_json_array_impl*,
				     unsigned int);
	void (*set)(struct b6_json_array_impl*, unsigned int,
		    struct b6_json_value*);
};

static inline unsigned int b6_json_array_len(const struct b6_json_array *self)
{
	return self->impl->ops->len(self->impl);
}

static inline struct b6_json_value *b6_json_get_array(
	const struct b6_json_array *self,
	unsigned int index)
{
	b6_precond(index < b6_json_array_len(self));
	return self->impl->ops->get(self->impl, index);
}

static inline void b6_json_set_array(struct b6_json_array *self,
					unsigned int index,
					struct b6_json_value *value)
{
	b6_precond(index < b6_json_array_len(self));
	self->impl->ops->set(self->impl, index, value);
}

static inline enum b6_json_error b6_json_add_array(struct b6_json_array *self,
						   unsigned int index,
						   struct b6_json_value *value)
{
	return self->impl->ops->add(self->impl, index, value);
}

static inline void b6_json_del_array(struct b6_json_array *self,
				     unsigned int index)
{
	b6_precond(index < b6_json_array_len(self));
	self->impl->ops->del(self->impl, index);
}

static inline struct b6_json_array *b6_json_new_array(struct b6_json *json)
{
	struct b6_json_array *self;
	if (!(self = b6_pool_get(&json->pool)))
		return NULL;
	if (!(self->impl = json->impl->ops->array_impl(json->impl))) {
		b6_pool_put(&json->pool, self);
		return NULL;
	}
	b6_json_setup_value(&self->up, &b6_json_array_ops);
	self->json = json;
	return self;
}

struct b6_json_object_impl {
	const struct b6_json_object_impl_ops *ops;
};

struct b6_json_object_impl_ops {
	void (*dtor)(struct b6_json_object_impl*, struct b6_json_impl*);
	enum b6_json_error (*add)(struct b6_json_object_impl*,
				  struct b6_json_impl*,
				  const struct b6_json_pair *pair);
	enum b6_json_error (*del)(struct b6_json_object_impl*,
				  struct b6_json_impl*,
				  struct b6_json_pair *pair);
	struct b6_json_pair *(*first)(struct b6_json_object_impl*);
	struct b6_json_pair *(*at)(struct b6_json_object_impl*,
				   const char*);
	struct b6_json_pair *(*at_utf8)(struct b6_json_object_impl*,
					const void*, unsigned int);
	struct b6_json_pair *(*walk)(struct b6_json_object_impl*,
				     struct b6_json_pair*, int);
};

static inline void b6_json_setup_iterator(struct b6_json_iterator *self,
					  const struct b6_json_object *object)
{
	self->object = object;
	self->pair = object->impl->ops->first(object->impl);
}

static inline void b6_json_setup_iterator_at(struct b6_json_iterator *self,
					     const struct b6_json_object *obj,
					     const char *key)
{
	self->object = obj;
	self->pair = obj->impl->ops->at(obj->impl, key);
}

static inline void b6_json_setup_iterator_at_utf8(
	struct b6_json_iterator *self,
	const struct b6_json_object *object,
	const void *utf8,
	unsigned int size)
{
	self->object = object;
	self->pair = object->impl->ops->at_utf8(object->impl, utf8, size);
}

static inline const struct b6_json_pair *b6_json_get_iterator(
	const struct b6_json_iterator *self)
{
	return self->pair;
}

static inline void b6_json_advance_iterator(struct b6_json_iterator *self)
{
	self->pair = self->object->impl->ops->walk(self->object->impl,
						   self->pair, B6_NEXT);
}

static inline struct b6_json_value *b6_json_get_object(
	const struct b6_json_object *self,
	const char *key)
{
	struct b6_json_iterator iter;
	const struct b6_json_pair *pair;
	b6_json_setup_iterator_at(&iter, self, key);
	pair = b6_json_get_iterator(&iter);
	return pair ? pair->value : NULL;
}

static inline struct b6_json_value *b6_json_get_object_utf8(
	const struct b6_json_object *self,
	const void *key,
	unsigned int len)
{
	struct b6_json_iterator iter;
	const struct b6_json_pair *pair;
	b6_json_setup_iterator_at_utf8(&iter, self, key, len);
	pair = b6_json_get_iterator(&iter);
	return pair ? pair->value : NULL;
}

#define b6_json_get_object_as(_self, _key, _type) \
	b6_json_value_as_or_null(b6_json_get_object(_self, _key), _type)

#define b6_json_get_object_utf8_as(_self, _key, _len, _type) \
	b6_json_value_as_or_null( \
		b6_json_get_object_utf8(_self, _key, _len), _type)

static inline enum b6_json_error b6_json_set_object(
	struct b6_json_object *self,
	const char *key,
	struct b6_json_value *value)
{
	struct b6_json_object_impl *impl = self->impl;
	struct b6_json_pair *pair =
		(struct b6_json_pair*)impl->ops->at(impl, key);
	struct b6_json_pair temp;
	enum b6_json_error error;
	if (pair) {
		b6_json_unref_value(pair->value);
		pair->value = value;
		return B6_JSON_OK;
	}
	if (!(temp.key = b6_json_new_string(self->json, key)))
		return B6_JSON_ALLOC_ERROR;
	temp.value = value;
	error = self->impl->ops->add(self->impl, self->json->impl, &temp);
	if (error)
		b6_json_unref_value(&temp.key->up);
	return error;
}

static inline enum b6_json_error b6_json_set_object_utf8(
	struct b6_json_object *self,
	const void *key, unsigned int len,
	struct b6_json_value *value)
{
	struct b6_json_object_impl *impl = self->impl;
	struct b6_json_pair *pair =
		(struct b6_json_pair*)impl->ops->at_utf8(impl, key, len);
	struct b6_json_pair temp;
	enum b6_json_error error;
	if (pair) {
		b6_json_unref_value(pair->value);
		pair->value = value;
		return B6_JSON_OK;
	}
	if (!(temp.key = b6_json_new_string(self->json, NULL)))
		return B6_JSON_ALLOC_ERROR;
	if ((error = b6_json_set_string(temp.key, key, len)))
		goto bail_out;
	temp.value = value;
	error = self->impl->ops->add(self->impl, self->json->impl, &temp);
bail_out:
	if (error)
		b6_json_unref_value(&temp.key->up);
	return error;
}

static inline enum b6_json_error b6_json_serialize_object(
	struct b6_json_object *self, struct b6_json_ostream *os,
	struct b6_json_serializer *serializer)
{
	return b6_json_serialize_value(&self->up, os, serializer);
}

extern enum b6_json_error b6_json_parse_object(struct b6_json_object*,
					       struct b6_json_istream*,
					       struct b6_json_parser_info*);

static inline struct b6_json_object *b6_json_new_object(struct b6_json *json)
{
	struct b6_json_object *self;
	if (!(self = b6_pool_get(&json->pool)))
		return NULL;
	if (!(self->impl = json->impl->ops->object_impl(json->impl))) {
		b6_pool_put(&json->pool, self);
		return NULL;
	}
	b6_json_setup_value(&self->up, &b6_json_object_ops);
	self->json = json;
	return self;
}

struct b6_json_default_impl {
	struct b6_json_impl up;
	struct b6_pool pair_pool;
	struct b6_pool array_pool;
	struct b6_pool string_pool;
	struct b6_pool object_pool;
	struct b6_pool pool;
	struct b6_allocator *allocator;
};

extern int b6_json_default_impl_initialize(struct b6_json_default_impl *self,
					   struct b6_allocator *allocator);

extern void b6_json_default_impl_finalize(struct b6_json_default_impl *self);

#endif /* B6_JSON_H */
