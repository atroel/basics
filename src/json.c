#include "b6/json.h"

#include "b6/array.h"
#include "b6/pool.h"
#include "b6/registry.h"
#include "b6/utf8.h"

static const char null_token[] = "null";
static const char true_token[] = "true";
static const char false_token[] = "false";
static const char opening_bracket = '[';
static const char closing_bracket = ']';
static const char opening_brace = '{';
static const char closing_brace = '}';
static const char colon = ':';
static const char comma = ',';
static const char quote = '"';
static const char point = '.';
static const char power = 'e';
static const char minus = '-';
static const char plus = '+';
static const char slash = '/';
static const char backslash = '\\';
static const char backspace = '\b';
static const char formfeed = '\f';
static const char newline = '\n';
static const char carriage_return = '\r';
static const char tab = '\t';

static int b6_json_is_whitespace(char c)
{
	switch (c) {
	case ' ':
	case '\t':
	case '\f':
	case '\r':
	case '\n':
		return 1;
	default:
		return 0;
	}
}

static long int b6_json_istream_read(struct b6_json_istream *self,
				     char *buf, unsigned long int len)
{
	char * const ptr = buf;
	if (!len)
		goto done;
	if (self->has_c) {
		self->has_c = 0;
		*buf++ = self->c;
		len -= 1;
	}
	if (len) {
		long int retval = self->ops->read(self, buf, len);
		if (retval < 0)
			return retval;
		buf += retval;
	}
done:
	return buf - ptr;
}

static int b6_json_istream_get(struct b6_json_istream *self, char *c,
			       struct b6_json_parser_info *info)
{
	if (self->has_c) {
		*c = self->c;
		self->has_c = 0;
	} else if (self->ops->read(self, c, sizeof(*c)) != sizeof(*c))
		return 0;
	if (info) {
		info->col += 1;
		if (*c == '\n') {
			info->row += 1;
			info->col = 0;
		}
	}
	return 1;
}

static enum b6_json_error b6_json_istream_unget(
	struct b6_json_istream *self, char c, struct b6_json_parser_info *info)
{
	if (self->has_c)
		return B6_JSON_ERROR;
	self->has_c = 1;
	self->c = c;
	if (info) {
		if (c == '\n')
			info->row -= 1;
		else
			info->col -= 1;
	}
	return B6_JSON_OK;
}

static long int b6_json_ostream_write(struct b6_json_ostream *self,
				      const void *buf, unsigned long int len)
{
	return self->ops->write(self, buf, len);
}

static int b6_json_ostream_flush(struct b6_json_ostream *self)
{
	return self->ops->flush(self);
}

static enum b6_json_error b6_json_istream_token(
	struct b6_json_istream *self, char *c, struct b6_json_parser_info *info)
{
	while (b6_json_istream_get(self, c, info))
		if (!b6_json_is_whitespace(*c))
			return B6_JSON_OK;
	return B6_JSON_IO_ERROR;
}

static enum b6_json_error serialize_null(const struct b6_json_value *up,
					 struct b6_json_ostream *os,
					 struct b6_json_serializer *helper)
{
	static const long int len = sizeof(null_token) - 1;
	if (b6_json_ostream_write(os, null_token, len) != len)
		return B6_JSON_IO_ERROR;
	return B6_JSON_OK;
}

const struct b6_json_value_ops b6_json_null_ops = {
	.serialize = serialize_null,
};

static enum b6_json_error serialize_true(const struct b6_json_value *up,
					 struct b6_json_ostream *os,
					 struct b6_json_serializer *helper)
{
	static const long int len = sizeof(true_token) - 1;
	if (b6_json_ostream_write(os, true_token, len) != len)
		return B6_JSON_IO_ERROR;
	return B6_JSON_OK;
}

const struct b6_json_value_ops b6_json_true_ops = {
	.serialize = serialize_true,
};

static enum b6_json_error serialize_false(const struct b6_json_value *up,
					  struct b6_json_ostream *os,
					  struct b6_json_serializer *helper)
{
	static const long int len = sizeof(false_token) - 1;
	if (b6_json_ostream_write(os, false_token, len) != len)
		return B6_JSON_IO_ERROR;
	return B6_JSON_OK;
}

const struct b6_json_value_ops b6_json_false_ops = {
	.serialize = serialize_false,
};

static enum b6_json_error serialize_uint(unsigned long long int u,
					 struct b6_json_ostream *os)
{
	char tmp[32];
	char *const end = tmp + sizeof(tmp);
	char *ptr = end;
	long int len;
	do {
		unsigned int d = u % 10;
		u /= 10;
		*--ptr = d + '0';
	} while (u > 0);
	len = end - ptr;
	return b6_json_ostream_write(os, ptr, len) != len ? B6_JSON_IO_ERROR :
		B6_JSON_OK;
}

static enum b6_json_error serialize_number(const struct b6_json_value *up,
					   struct b6_json_ostream *os,
					   struct b6_json_serializer *helper)
{
	const struct b6_json_number *self = b6_json_value_as(up, number);
	double epsilon = 1e-12;
	double d;
	unsigned long long int u;
	int e;
	enum b6_json_error retval;
	d = self->number;
	if (d < 0) {
		d = -d;
		if (b6_json_ostream_write(os, &minus, 1) != 1)
			return B6_JSON_IO_ERROR;
	}
	if (d == (double)(u = (unsigned long long int)d))
		return serialize_uint(u, os);
	e = 0;
	if (d < 1)
		do {
			d *= 10;
			e -= 1;
		} while (d < 1);
	else
		while (d > 10) {
			d /= 10;
			e += 1;
		}
	u = (unsigned long long int)d;
	d -= (double)u;
	if (d >= 1 - epsilon) {
		u += 1;
		d = 0;
	}
	if ((retval = serialize_uint(u, os)))
		return retval;
	if (d > 0) {
		if (b6_json_ostream_write(os, &point, 1) != 1)
			return B6_JSON_IO_ERROR;
		d = ((unsigned long long int)(d / epsilon)) * epsilon;
		for (;;) {
			char c;
			d *= 10;
			epsilon *= 10;
			u = (unsigned long long int)d;
			d -= (double)u;
			if (d >= 1 - epsilon) {
				u += 1;
				if ((retval = serialize_uint(u, os)))
					return retval;
				break;
			}
			c = u + '0';
			if (b6_json_ostream_write(os, &c, 1) != 1)
				return B6_JSON_IO_ERROR;
		}
	}
	if (!e)
		return B6_JSON_OK;
	if (b6_json_ostream_write(os, &power, 1) != 1)
		return B6_JSON_IO_ERROR;
	if (e < 0) {
		e = -e;
		if (b6_json_ostream_write(os, &minus, 1) != 1)
			return B6_JSON_IO_ERROR;
	}
	return serialize_uint(e, os);
}

static void number_dtor(struct b6_json_value *up)
{
	struct b6_json_number *self = b6_json_value_as(up, number);
	b6_pool_put(&self->json->pool, self);
}

const struct b6_json_value_ops b6_json_number_ops = {
	.dtor = number_dtor,
	.serialize = serialize_number,
};

static enum b6_json_error serialize_array(const struct b6_json_value *up,
					  struct b6_json_ostream *os,
					  struct b6_json_serializer *helper)
{
	struct b6_json_array *self = b6_cast_of(up, struct b6_json_array, up);
	unsigned int index = 0, len = b6_json_array_len(self);
	enum b6_json_error retval;
	if ((retval = helper->ops->enter_array(helper, os, self)))
		return retval;
	if (len) for (;;) {
		struct b6_json_value *value = b6_json_get_array(self, index++);
		int last = index == len;
		if ((retval = helper->ops->enter_array_value(helper, os,
							     value)))
			return retval;
		if ((retval = helper->ops->leave_array_value(helper, os, value,
							     last)))
			return retval;
		if (last)
			break;
	}
	return helper->ops->leave_array(helper, os, self);
}

static void array_dtor(struct b6_json_value *up)
{
	struct b6_json_array *self = b6_json_value_as(up, array);
	self->impl->ops->dtor(self->impl, self->json->impl);
	b6_pool_put(&self->json->pool, self);
}

const struct b6_json_value_ops b6_json_array_ops = {
	.dtor = array_dtor,
	.serialize = serialize_array,
};

static enum b6_json_error serialize_object(const struct b6_json_value *up,
					   struct b6_json_ostream *os,
					   struct b6_json_serializer *helper)
{
	struct b6_json_object *self = b6_cast_of(up, struct b6_json_object, up);
	struct b6_json_iterator iter;
	const struct b6_json_pair *curr, *next;
	enum b6_json_error retval;
	if ((retval = helper->ops->enter_object(helper, os, self)))
		return retval;
	b6_json_setup_iterator(&iter, self);
	for (curr = b6_json_get_iterator(&iter); curr; curr = next) {
		b6_json_advance_iterator(&iter);
		next = b6_json_get_iterator(&iter);
		if ((retval = helper->ops->enter_object_key(helper, os,
							    curr->key)))
			return retval;
		if ((retval = helper->ops->leave_object_key(helper, os,
							    curr->key)))
			return retval;
		if ((retval = helper->ops->enter_object_value(helper, os,
							      curr->value)))
			return retval;
		if ((retval = helper->ops->leave_object_value(helper, os,
							      curr->value,
							      !next)))
			return retval;
	}
	return helper->ops->leave_object(helper, os, self);
}

static void object_dtor(struct b6_json_value *up)
{
	struct b6_json_object *self = b6_json_value_as(up, object);
	self->impl->ops->dtor(self->impl, self->json->impl);
	b6_pool_put(&self->json->pool, self);
}

const struct b6_json_value_ops b6_json_object_ops = {
	.dtor = object_dtor,
	.serialize = serialize_object,
};

struct b6_json_array_default_impl {
	struct b6_json_array_impl up;
	struct b6_array array;
};

static unsigned int array_default_impl_len(const struct b6_json_array_impl *up)
{
	const struct b6_json_array_default_impl *self =
		b6_cast_of(up, struct b6_json_array_default_impl, up);
	return b6_array_length(&self->array);
}

static struct b6_json_value *array_default_impl_get(
	const struct b6_json_array_impl *up, unsigned int index)
{
	struct b6_json_array_default_impl *self =
		b6_cast_of(up, struct b6_json_array_default_impl, up);
	return *(struct b6_json_value**)b6_array_get(&self->array, index);
}

static void array_default_impl_set(struct b6_json_array_impl *up,
				   unsigned int index,
				   struct b6_json_value *value)
{
	struct b6_json_array_default_impl *self =
		b6_cast_of(up, struct b6_json_array_default_impl, up);
	struct b6_json_value **ptr = b6_array_get(&self->array, index);
	b6_json_unref_value(*ptr);
	*ptr = value;
}

static int array_default_impl_add(struct b6_json_array_impl *up,
				  unsigned int index,
				  struct b6_json_value *new_value)
{
	struct b6_json_array_default_impl *self =
		b6_cast_of(up, struct b6_json_array_default_impl, up);
	struct b6_json_value **ptr = b6_array_extend(&self->array, 1);
	unsigned int len = array_default_impl_len(up);
	if (!ptr)
		return B6_JSON_ALLOC_ERROR;
	if (index >= len)
		index = len;
	while (--len > index) {
		ptr -= 1;
		ptr[1] = ptr[0];
	}
	*ptr = new_value;
	return B6_JSON_OK;
}

static void array_default_impl_del(struct b6_json_array_impl *up,
				   unsigned int index)
{
	struct b6_json_array_default_impl *self =
		b6_cast_of(up, struct b6_json_array_default_impl, up);
	unsigned int len = array_default_impl_len(up);
	struct b6_json_value **ptr;
	if (index >= len)
		return;
	ptr = (struct b6_json_value**)b6_array_get(&self->array, index);
	b6_json_unref_value(*ptr);
	while (++index < len) {
		ptr[0] = ptr[1];
		ptr += 1;
	}
	b6_array_reduce(&self->array, 1);
}

static void array_default_impl_dtor(struct b6_json_array_impl *up,
				    struct b6_json_impl *impl)
{
	struct b6_json_array_default_impl *self =
		b6_cast_of(up, struct b6_json_array_default_impl, up);
	struct b6_json_default_impl *default_impl =
		b6_cast_of(impl, struct b6_json_default_impl, up);
	struct b6_json_value **values = b6_array_get(&self->array, 0);
	unsigned int i = b6_array_length(&self->array);
	while (i--)
		b6_json_unref_value(*values++);
	b6_array_finalize(&self->array);
	b6_pool_put(&default_impl->array_pool, self);
}

static struct b6_json_array_impl *array_default_impl_new(
	struct b6_json_impl *up)
{
	struct b6_json_default_impl *self =
		b6_cast_of(up, struct b6_json_default_impl, up);
	static const struct b6_json_array_impl_ops ops = {
		.dtor = array_default_impl_dtor,
		.len = array_default_impl_len,
		.add = array_default_impl_add,
		.del = array_default_impl_del,
		.get = array_default_impl_get,
		.set = array_default_impl_set,
	};
	struct b6_json_array_default_impl *impl;
	if (!(impl = b6_pool_get(&self->array_pool)))
		return NULL;
	impl->up.ops = &ops;
	b6_array_initialize(&impl->array, self->allocator,
			    sizeof(struct b6_json_value*));
	return &impl->up;
}

struct b6_json_string_default_impl {
	struct b6_json_string_impl up;
	struct b6_utf8_string utf8_string;
};

static void string_default_impl_dtor(struct b6_json_string_impl *up,
				     struct b6_json_impl *impl)
{
	struct b6_json_string_default_impl *self =
		b6_cast_of(up, struct b6_json_string_default_impl, up);
	struct b6_json_default_impl *default_impl =
		b6_cast_of(impl, struct b6_json_default_impl, up);
	b6_finalize_utf8_string(&self->utf8_string);
	b6_pool_put(&default_impl->string_pool, self);
}

static const struct b6_utf8 *string_default_impl_get(
	const struct b6_json_string_impl *up)
{
	struct b6_json_string_default_impl *self = b6_cast_of(
		up, struct b6_json_string_default_impl, up);
	return &self->utf8_string.utf8;
}

static enum b6_json_error string_default_impl_append(
	struct b6_json_string_impl *up,
	struct b6_json_impl *impl,
	const struct b6_utf8 *utf8)
{
	struct b6_json_string_default_impl *self = b6_cast_of(
		up, struct b6_json_string_default_impl, up);
	if (b6_extend_utf8_string(&self->utf8_string, utf8))
		return B6_JSON_ALLOC_ERROR;
	return B6_JSON_OK;
}

static struct b6_json_string_impl *string_default_impl_new(
	struct b6_json_impl *up, const struct b6_utf8 *utf8)
{
	struct b6_json_default_impl *self =
		b6_cast_of(up, struct b6_json_default_impl, up);
	static const struct b6_json_string_impl_ops ops = {
		.dtor = string_default_impl_dtor,
		.get = string_default_impl_get,
		.append = string_default_impl_append,
	};
	struct b6_json_string_default_impl *impl;
	if (!(impl = b6_pool_get(&self->string_pool)))
		return NULL;
	impl->up.ops = &ops;
	b6_initialize_utf8_string(&impl->utf8_string, self->allocator);
	if (utf8 && b6_extend_utf8_string(&impl->utf8_string, utf8)) {
		b6_pool_put(&self->string_pool, impl);
		return NULL;
	}
	return &impl->up;
}

static void string_dtor(struct b6_json_value *up)
{
	struct b6_json_string *self = b6_cast_of(up, struct b6_json_string, up);
	self->impl->ops->dtor(self->impl, self->json->impl);
	b6_pool_put(&self->json->pool, self);
}

static enum b6_json_error serialize_string(const struct b6_json_value *up,
					   struct b6_json_ostream *os,
					   struct b6_json_serializer *helper)
{
	struct b6_json_string *self = b6_cast_of(up, struct b6_json_string, up);
	struct b6_json_string_default_impl *impl = b6_cast_of(
		self->impl, struct b6_json_string_default_impl, up);
	struct b6_utf8_iterator iter;
	b6_setup_utf8_iterator(&iter, &impl->utf8_string.utf8);
	if (b6_json_ostream_write(os, &quote, 1) != 1)
		return B6_JSON_IO_ERROR;
	while (b6_utf8_iterator_has_next(&iter)) {
		unsigned int unicode = b6_utf8_iterator_get_next(&iter);
		if (unicode >= 128) {
			unsigned char utf8[4];
			int len = b6_utf8_enc_len(unicode);
			b6_assert(len > 0);
			b6_utf8_enc(len, unicode, utf8);
			if (b6_json_ostream_write(os, utf8, len) != len)
				return B6_JSON_IO_ERROR;
		} else {
			char escape[2] = { '\\', '\0' };
			escape[1] = unicode;
			switch (escape[1]) {
			case '\"':
			case '\\':
				if (b6_json_ostream_write(os, escape, 2) != 2)
					return B6_JSON_IO_ERROR;
				break;
			case '\b':
				escape[1] = 'b';
				if (b6_json_ostream_write(os, escape, 2) != 2)
					return B6_JSON_IO_ERROR;
				break;
			case '\f':
				escape[1] = 'f';
				if (b6_json_ostream_write(os, escape, 2) != 2)
					return B6_JSON_IO_ERROR;
				break;
			case '\n':
				escape[1] = 'n';
				if (b6_json_ostream_write(os, escape, 2) != 2)
					return B6_JSON_IO_ERROR;
				break;
			case '\r':
				escape[1] = 'r';
				if (b6_json_ostream_write(os, escape, 2) != 2)
					return B6_JSON_IO_ERROR;
				break;
			case '\t':
				escape[1] = 't';
				if (b6_json_ostream_write(os, escape, 2) != 2)
					return B6_JSON_IO_ERROR;
				break;
			default:
				if (b6_json_ostream_write(os, escape + 1,
							  1) != 1)
					return B6_JSON_IO_ERROR;
			}
		}
	}
	if (b6_json_ostream_write(os, &quote, 1) != 1)
		return B6_JSON_IO_ERROR;
	return B6_JSON_OK;
}

const struct b6_json_value_ops b6_json_string_ops = {
	.dtor = string_dtor,
	.serialize = serialize_string,
};

struct b6_json_object_default_impl {
	struct b6_json_object_impl up;
	struct b6_registry registry;
};

struct b6_json_pair_default {
	struct b6_entry entry;
	struct b6_json_pair pair;
};

static enum b6_json_error object_default_impl_add(
	struct b6_json_object_impl *up,
	struct b6_json_impl *impl,
	const struct b6_json_pair *pair)
{
	struct b6_json_object_default_impl *self =
		b6_cast_of(up, struct b6_json_object_default_impl, up);
	struct b6_json_default_impl *default_impl =
		b6_cast_of(impl, struct b6_json_default_impl, up);
	struct b6_json_pair_default *default_pair =
		b6_pool_get(&default_impl->pair_pool);
	const struct b6_utf8 *utf8 = b6_json_get_string(pair->key);
	if (!default_pair)
		return B6_JSON_ALLOC_ERROR;
	if (b6_register(&self->registry, &default_pair->entry, utf8)) {
		b6_pool_put(&default_impl->pair_pool, default_pair);
		return B6_JSON_ERROR;
	}
	default_pair->pair.key = pair->key;
	default_pair->pair.value = pair->value;
	return B6_JSON_OK;
}

static enum b6_json_error object_default_impl_del(
	struct b6_json_object_impl *up,
	struct b6_json_impl *impl,
	const struct b6_json_pair *pair)
{
	struct b6_json_object_default_impl *self =
		b6_cast_of(up, struct b6_json_object_default_impl, up);
	struct b6_json_default_impl *default_impl =
		b6_cast_of(impl, struct b6_json_default_impl, up);
	struct b6_json_pair_default *default_pair =
		b6_cast_of(pair, struct b6_json_pair_default, pair);
	b6_unregister(&self->registry, &default_pair->entry);
	b6_json_unref_value(&default_pair->pair.key->up);
	b6_json_unref_value(default_pair->pair.value);
	b6_pool_put(&default_impl->pair_pool, default_pair);
	return B6_JSON_OK;
}

static struct b6_json_pair *object_default_impl_first(
	struct b6_json_object_impl *up)
{
	struct b6_json_object_default_impl *self =
		b6_cast_of(up, struct b6_json_object_default_impl, up);
	struct b6_entry *entry = b6_get_first_entry(&self->registry);
	if (!entry)
		return NULL;
	return &b6_cast_of(entry, struct b6_json_pair_default, entry)->pair;
}

static struct b6_json_pair *object_default_impl_walk(
	struct b6_json_object_impl *up,
	struct b6_json_pair *pair,
	int dir)
{
	struct b6_json_object_default_impl *self =
		b6_cast_of(up, struct b6_json_object_default_impl, up);
	struct b6_entry *entry =
		&b6_cast_of(pair, struct b6_json_pair_default, pair)->entry;
	entry = b6_walk_registry(&self->registry, entry, dir);
	if (!entry)
		return NULL;
	return &b6_cast_of(entry, struct b6_json_pair_default, entry)->pair;
}

static struct b6_json_pair *object_default_impl_at(
	struct b6_json_object_impl *up,
	const struct b6_utf8 *key)
{
	struct b6_json_object_default_impl *self =
		b6_cast_of(up, struct b6_json_object_default_impl, up);
	struct b6_entry *entry = b6_lookup_registry(&self->registry, key);
	if (!entry)
		return NULL;
	return &b6_cast_of(entry, struct b6_json_pair_default, entry)->pair;
}

static void object_default_impl_dtor(struct b6_json_object_impl *up,
				     struct b6_json_impl *impl)
{
	struct b6_json_object_default_impl *self =
		b6_cast_of(up, struct b6_json_object_default_impl, up);
	struct b6_json_default_impl *default_impl =
		b6_cast_of(impl, struct b6_json_default_impl, up);
	struct b6_json_pair *pair = object_default_impl_first(up);
	while (pair) {
		struct b6_json_pair *next =
			object_default_impl_walk(up, pair, B6_NEXT);
		object_default_impl_del(up, impl, pair);
		pair = next;
	}
	b6_pool_put(&default_impl->object_pool, self);
}

static struct b6_json_object_impl *object_default_impl_new(
	struct b6_json_impl *up)
{
	struct b6_json_default_impl *self =
		b6_cast_of(up, struct b6_json_default_impl, up);
	static const struct b6_json_object_impl_ops ops = {
		.dtor = object_default_impl_dtor,
		.add = object_default_impl_add,
		.del = object_default_impl_del,
		.at = object_default_impl_at,
		.first = object_default_impl_first,
		.walk = object_default_impl_walk,
	};
	struct b6_json_object_default_impl *impl;
	if (!(impl = b6_pool_get(&self->object_pool)))
		return NULL;
	impl->up.ops = &ops;
	b6_setup_registry(&impl->registry);
	return &impl->up;
}

#define JSON_IMPL_SIZE (4096 - sizeof(void*))

int b6_json_default_impl_initialize(struct b6_json_default_impl *self,
				    struct b6_allocator *allocator)
{
	static const struct b6_json_impl_ops b6_json_impl_ops = {
		.array_impl = array_default_impl_new,
		.string_impl = string_default_impl_new,
		.object_impl = object_default_impl_new,
	};
	static const unsigned int s = JSON_IMPL_SIZE;
	static const unsigned int p =
		(JSON_IMPL_SIZE - sizeof(struct b6_chunk)) / 4;
	self->up.ops = &b6_json_impl_ops;
	self->allocator = allocator;
	b6_pool_initialize(&self->pool, allocator, p, s);
	b6_pool_initialize(&self->pair_pool, &self->pool.parent,
			   sizeof(struct b6_json_pair_default), p);
	b6_pool_initialize(&self->array_pool, &self->pool.parent,
			   sizeof(struct b6_json_array_default_impl), p);
	b6_pool_initialize(&self->string_pool, &self->pool.parent,
			   sizeof(struct b6_json_string_default_impl), p);
	b6_pool_initialize(&self->object_pool, &self->pool.parent,
			   sizeof(struct b6_json_object_default_impl), p);
	return 0;
}

void b6_json_default_impl_finalize(struct b6_json_default_impl *self)
{
	b6_pool_finalize(&self->pair_pool);
	b6_pool_finalize(&self->array_pool);
	b6_pool_finalize(&self->string_pool);
	b6_pool_finalize(&self->object_pool);
	b6_pool_finalize(&self->pool);
}

static enum b6_json_error parse_string(struct b6_json_string *self,
				       struct b6_json_istream *is,
				       struct b6_json_parser_info *info)
{
	enum b6_json_error retval = B6_JSON_OK;
	char hex[4];
	char c;
	int i;
	for (;;) {
		struct b6_utf8 utf8;
		unsigned int unicode;
		int utf8_len;
		char utf8_buf[4];
		if (!b6_json_istream_get(is, &c, info)) {
			retval = B6_JSON_IO_ERROR;
			break;
		}
		if (c == quote)
			break;
		if (c != backslash) {
			int ret;
			utf8_len = b6_utf8_dec_len(&c);
			if (utf8_len < 1) {
				retval = B6_JSON_PARSE_ERROR;
				break;
			}
			ret = b6_json_istream_read(is, &utf8_buf[1],
						   utf8_len - 1);
			if (ret != utf8_len - 1) {
				retval = B6_JSON_IO_ERROR;
				break;
			}
			utf8_buf[0] = c;
			if (b6_utf8_dec(utf8_len, &unicode, utf8_buf) !=
			    utf8_len) {
				retval = B6_JSON_PARSE_ERROR;
				break;
			}
			b6_setup_utf8(&utf8, utf8_buf, utf8_len);
			if ((retval = self->impl->ops->append(self->impl,
							      self->json->impl,
							      &utf8)))
				break;
			continue;
		}
		if (!b6_json_istream_get(is, &c, info)) {
			retval = B6_JSON_IO_ERROR;
			break;
		}
		if (c == quote || c == backslash || c == slash) {
			retval = self->impl->ops->append(
				self->impl, self->json->impl, &b6_utf8_char[c]);
			if (retval)
				break;
			continue;
		}
		if (c == 'b') {
			retval = self->impl->ops->append(
				self->impl, self->json->impl,
				&b6_utf8_char[backspace]);
			if (retval)
				break;
			continue;
		}
		if (c == 'f') {
			retval = self->impl->ops->append(
				self->impl, self->json->impl,
				&b6_utf8_char[formfeed]);
			if (retval)
				break;
			continue;
		}
		if (c == 'n') {
			retval = self->impl->ops->append(
				self->impl, self->json->impl,
				&b6_utf8_char[newline]);
			if (retval)
				break;
			continue;
		}
		if (c == 'r') {
			retval = self->impl->ops->append(
				self->impl, self->json->impl,
				&b6_utf8_char[carriage_return]);
			if (retval)
				break;
			continue;
		}
		if (c == 't') {
			retval = self->impl->ops->append(self->impl,
							 self->json->impl,
							 &b6_utf8_char[tab]);
			if (retval)
				break;
			continue;
		}
		if (c != 'u') {
			retval = B6_JSON_PARSE_ERROR;
			break;
		}
		if (b6_json_istream_read(is, hex, sizeof(hex)) != sizeof(hex)) {
			retval = B6_JSON_IO_ERROR;
			break;
		}
		for (i = 0, unicode = 0; i < sizeof(hex); i += 1) {
			unicode *= 16;
			switch (hex[i]) {
			case '0': case '1': case '2': case '3': case '4':
			case '5': case '6': case '7': case '8': case '9':
				unicode += hex[i] - '0';
				break;
			case 'a': case 'b': case 'c':
			case 'd': case 'e': case 'f':
				unicode += hex[i] - 'a' + 10;
				break;
			case 'A': case 'B': case 'C':
			case 'D': case 'E': case 'F':
				unicode += hex[i] - 'A' + 10;
				break;
			default:
				return B6_JSON_PARSE_ERROR;
			}
		}
		utf8_len = b6_utf8_enc_len(unicode);
		if (utf8_len < 1)
			return B6_JSON_PARSE_ERROR;
		if (b6_utf8_enc(utf8_len, unicode, utf8_buf) != utf8_len)
			continue;
		b6_setup_utf8(&utf8, utf8_buf, utf8_len);
		retval = self->impl->ops->append(self->impl, self->json->impl,
						 &utf8);
		if (retval)
			break;
	}
	return retval;
}

static enum b6_json_error parse_value(struct b6_json*, struct b6_json_istream*,
				      struct b6_json_value**,
				      struct b6_json_parser_info*);

static enum b6_json_error parse_object(struct b6_json_object *self,
				       struct b6_json_istream *is,
				       struct b6_json_parser_info *info)
{
	char c;
	enum b6_json_error retval;
	if ((retval = b6_json_istream_token(is, &c, info)) != B6_JSON_OK)
		return retval;
	/* Nicely accept a comma before the closing brace. */
	while (c != closing_brace) {
		struct b6_json_pair pair;
		if (c != quote)
			return B6_JSON_PARSE_ERROR;
		if (!(pair.key = b6_json_new_string(self->json, NULL)))
			return B6_JSON_PARSE_ERROR;
		if ((retval = parse_string(pair.key, is, info)) != B6_JSON_OK)
			return retval;
		if ((retval = b6_json_istream_token(is, &c, info)))
			return retval;
		if (c != colon)
			return B6_JSON_PARSE_ERROR;
		if ((retval = parse_value(self->json, is, &pair.value, info))) {
			b6_json_unref_value(&pair.key->up);
			return retval;
		}
		retval = self->impl->ops->add(self->impl, self->json->impl,
					      &pair);
		if (retval != B6_JSON_OK) {
			b6_json_unref_value(&pair.key->up);
			b6_json_unref_value(pair.value);
			return retval;
		}
		if ((retval = b6_json_istream_token(is, &c, info)))
			return retval;
		if (c == closing_brace)
			return B6_JSON_OK;
		if (c != comma)
			return B6_JSON_PARSE_ERROR;
		if ((retval = b6_json_istream_token(is, &c, info)))
			return retval;
	}
	return B6_JSON_OK;
}

static enum b6_json_error parse_value_inner(struct b6_json*,
					    struct b6_json_istream*,
					    char,
					    struct b6_json_value**,
					    struct b6_json_parser_info*);

static enum b6_json_error parse_array(struct b6_json_array *self,
				      struct b6_json_istream *is,
				      struct b6_json_parser_info *info)
{
	char c;
	if (b6_json_istream_token(is, &c, info) != B6_JSON_OK)
		return B6_JSON_IO_ERROR;
	unsigned int index = b6_json_array_len(self);
	/* Nicely accept a comma before the closing bracket. */
	while (c != closing_bracket) {
		struct b6_json_value *value;
		enum b6_json_error retval;
		if ((retval = parse_value_inner(self->json, is, c, &value,
						info)))
			return retval;
		if ((retval = b6_json_add_array(self, index++, value))) {
			b6_json_unref_value(value);
			return retval;
		}
		if ((retval = b6_json_istream_token(is, &c, info)))
			return retval;
		if (c == closing_bracket)
			break;
		if (c != comma)
			return B6_JSON_PARSE_ERROR;
		if ((retval = b6_json_istream_token(is, &c, info)))
			return retval;
	}
	return B6_JSON_OK;
}

static int is_digit(char c) { return c >= '0' && c <= '9'; }

static enum b6_json_error parse_number(struct b6_json_number *self, char c,
				       struct b6_json_istream *is,
				       struct b6_json_parser_info *info)
{
	double d;
	int neg = 0;
	if (c == minus) {
		neg = 1;
		if (!b6_json_istream_get(is, &c, info))
			return B6_JSON_IO_ERROR;
	}
	if (c == 0) {
		neg = 0;
		d = 0;
	} else if (is_digit(c)) {
		d = c - '0';
		for (;;) {
			if (!b6_json_istream_get(is, &c, info))
				return B6_JSON_IO_ERROR;
			if (!is_digit(c))
				break;
			d = d * 10 + c - '0';
		}
	} else
		return B6_JSON_PARSE_ERROR;
	if (c == point) {
		double f = 1;
		if (!b6_json_istream_get(is, &c, info))
			return B6_JSON_IO_ERROR;
		if (!is_digit(c))
			return B6_JSON_PARSE_ERROR;
		do {
			f /= 10;
			d += f * (c - '0');
			if (!b6_json_istream_get(is, &c, info))
				return B6_JSON_IO_ERROR;
		} while (is_digit(c));
	}
	if (c == power || c == (power & ~32)) {
		double e;
		if (!b6_json_istream_get(is, &c, info))
			return B6_JSON_IO_ERROR;
		if (c == minus) {
			if (!b6_json_istream_get(is, &c, info))
				return B6_JSON_IO_ERROR;
			e = -1;
		} else {
			e = 1;
			if (c == plus && !b6_json_istream_get(is, &c, info))
				return B6_JSON_IO_ERROR;
		}
		if (!is_digit(c))
			return B6_JSON_PARSE_ERROR;
		e *= c - '0';
		for (;;) {
			if (!b6_json_istream_get(is, &c, info))
				return B6_JSON_IO_ERROR;
			if (!is_digit(c))
				break;
			e = e * 10 + c - '0';
		}
		d *= e;
	}
	b6_json_istream_unget(is, c, info);
	if (neg)
		d = -d;
	b6_json_set_number(self, d);
	return B6_JSON_OK;
}

static enum b6_json_error parse_token(struct b6_json_istream *is,
				      const char *token,
				      struct b6_json_parser_info *info)
{
	char c;
	while (*token)
		if (!b6_json_istream_get(is, &c, info))
			return B6_JSON_IO_ERROR;
		else if (c != *token++)
			return B6_JSON_PARSE_ERROR;
	return B6_JSON_OK;
}

static enum b6_json_error parse_value_inner(struct b6_json *self,
					    struct b6_json_istream *is,
					    char c,
					    struct b6_json_value **value,
					    struct b6_json_parser_info *info)
{
	enum b6_json_error retval;
	if (c == *null_token) {
		struct b6_json_null *v = b6_json_new_null(self);
		if (!v)
			return B6_JSON_ALLOC_ERROR;
		*value = &v->up;
		retval = parse_token(is, null_token + 1, info);
	} else if (c == *true_token) {
		struct b6_json_true *v = b6_json_new_true(self);
		if (!v)
			return B6_JSON_ALLOC_ERROR;
		*value = &v->up;
		retval = parse_token(is, true_token + 1, info);
	} else if (c == *false_token) {
		struct b6_json_false *v = b6_json_new_false(self);
		if (!v)
			return B6_JSON_ALLOC_ERROR;
		*value = &v->up;
		retval = parse_token(is, false_token + 1, info);
	} else if (c == opening_bracket) {
		struct b6_json_array *v = b6_json_new_array(self);
		if (!v)
			return B6_JSON_ALLOC_ERROR;
		*value = &v->up;
		retval = parse_array(v, is, info);
	} else if (c == opening_brace) {
		struct b6_json_object *v = b6_json_new_object(self);
		if (!v)
			return B6_JSON_ALLOC_ERROR;
		*value = &v->up;
		retval = parse_object(v, is, info);
	} else if (c == quote) {
		struct b6_json_string *v = b6_json_new_string(self, NULL);
		if (!v)
			return B6_JSON_ALLOC_ERROR;
		*value = &v->up;
		retval = parse_string(v, is, info);
	} else {
		struct b6_json_number *v = b6_json_new_number(self, 0);
		if (!v)
			return B6_JSON_ALLOC_ERROR;
		*value = &v->up;
		retval = parse_number(v, c, is, info);
	}
	if (retval)
		b6_json_unref_value(*value);
	return retval;
}

static enum b6_json_error parse_value(struct b6_json *self,
				      struct b6_json_istream *is,
				      struct b6_json_value **value,
				      struct b6_json_parser_info *info)
{
	char c;
	enum b6_json_error retval = b6_json_istream_token(is, &c, info);
	return retval ? retval : parse_value_inner(self, is, c, value, info);
}

static void b6_json_object_swap(struct b6_json_object *lhs,
				struct b6_json_object *rhs)
{
	struct b6_json_object tmp;
	tmp.up.refcount = lhs->up.refcount;
	lhs->up.refcount = rhs->up.refcount;
	rhs->up.refcount = tmp.up.refcount;
	tmp.impl = lhs->impl;
	lhs->impl = rhs->impl;
	rhs->impl = tmp.impl;
	tmp.json = lhs->json;
	lhs->json = rhs->json;
	rhs->json = tmp.json;
}

enum b6_json_error b6_json_parse_object(struct b6_json_object *self,
					struct b6_json_istream *is,
					struct b6_json_parser_info *info)
{
	struct b6_json_object *temp;
	enum b6_json_error retval;
	char c;
	if ((retval = b6_json_istream_token(is, &c, info)))
		return retval;
	if (c != opening_brace)
		return B6_JSON_PARSE_ERROR;
	if (!(temp = b6_json_new_object(self->json)))
		return B6_JSON_ALLOC_ERROR;
	if ((retval = parse_object(temp, is, info)))
		return retval;
	b6_json_object_swap(self, temp);
	b6_json_unref_value(&temp->up);
	return B6_JSON_OK;
}

static enum b6_json_error enter_object(struct b6_json_serializer *up,
				       struct b6_json_ostream *os,
				       const struct b6_json_object *object)
{
	struct b6_json_default_serializer *self =
		b6_cast_of(up, struct b6_json_default_serializer, up);
	self->depth += 1;
	if (b6_json_ostream_write(os, &opening_brace, 1) != 1)
		return B6_JSON_IO_ERROR;
	return B6_JSON_OK;
}

static enum b6_json_error leave_object(struct b6_json_serializer *up,
				       struct b6_json_ostream *os,
				       const struct b6_json_object *object)
{
	struct b6_json_default_serializer *self =
		b6_cast_of(up, struct b6_json_default_serializer, up);
	if (b6_json_ostream_write(os, &closing_brace, 1) != 1)
		return B6_JSON_IO_ERROR;
	if (!--self->depth && b6_json_ostream_flush(os))
		return B6_JSON_IO_ERROR;
	return B6_JSON_OK;
}

static enum b6_json_error enter_object_key(struct b6_json_serializer *self,
					   struct b6_json_ostream *os,
					   const struct b6_json_string *key)
{
	return b6_json_serialize_value(&key->up, os, self);
}

static enum b6_json_error leave_object_key(struct b6_json_serializer *self,
					   struct b6_json_ostream *os,
					   const struct b6_json_string *key)
{
	if (b6_json_ostream_write(os, &colon, 1) != 1)
		return B6_JSON_IO_ERROR;
	return B6_JSON_OK;
}

static enum b6_json_error enter_object_value(struct b6_json_serializer *self,
					     struct b6_json_ostream *os,
					     const struct b6_json_value *value)
{
	return b6_json_serialize_value(value, os, self);
}

static enum b6_json_error leave_object_value(struct b6_json_serializer *self,
					     struct b6_json_ostream *os,
					     const struct b6_json_value *value,
					     int last)
{
	if (!last && b6_json_ostream_write(os, &comma, 1) != 1)
		return B6_JSON_IO_ERROR;
	return B6_JSON_OK;
}

static enum b6_json_error enter_array(struct b6_json_serializer *self,
				      struct b6_json_ostream *os,
				      const struct b6_json_array *array)
{
	if (b6_json_ostream_write(os, &opening_bracket, 1) != 1)
		return B6_JSON_IO_ERROR;
	return B6_JSON_OK;
}

static enum b6_json_error leave_array(struct b6_json_serializer *self,
				      struct b6_json_ostream *os,
				      const struct b6_json_array *array)
{
	if (b6_json_ostream_write(os, &closing_bracket, 1) != 1)
		return B6_JSON_IO_ERROR;
	return B6_JSON_OK;
}

static enum b6_json_error enter_array_value(struct b6_json_serializer *self,
					    struct b6_json_ostream *os,
					    const struct b6_json_value *value)
{
	return b6_json_serialize_value(value, os, self);
}

static enum b6_json_error leave_array_value(struct b6_json_serializer *self,
					    struct b6_json_ostream *os,
					    const struct b6_json_value *value,
					    int last)
{
	if (!last && b6_json_ostream_write(os, &comma, 1) != 1)
		return B6_JSON_IO_ERROR;
	return B6_JSON_OK;
}

const struct b6_json_serializer_ops b6_json_default_serializer_ops = {

	.enter_object = enter_object,
	.leave_object = leave_object,
	.enter_object_key = enter_object_key,
	.leave_object_key = leave_object_key,
	.enter_object_value = enter_object_value,
	.leave_object_value = leave_object_value,
	.enter_array = enter_array,
	.leave_array = leave_array,
	.enter_array_value = enter_array_value,
	.leave_array_value = leave_array_value,
};

const char *b6_json_strerror(enum b6_json_error error)
{
	switch (error) {
	case B6_JSON_OK: return "ok";
	case B6_JSON_ERROR: return "error";
	case B6_JSON_IO_ERROR: return "I/O error";
	case B6_JSON_ALLOC_ERROR: return "allocation error";
	case B6_JSON_PARSE_ERROR: return "parse error";
	default: return "unknown error";
	}
}
