/*
 * Copyright (c) 2009-2015, Arnaud TROEL
 * See LICENSE file for license details.
 */

#ifndef B6_UTF8_H
#define B6_UTF8_H

#include "b6/allocator.h"
#include "b6/utils.h"

static inline unsigned int b6_utf8_enc_len(unsigned int unicode)
{
	if (unicode < 0x80)
		return 1;
	if (unicode < 0x800)
		return 2;
	if (unicode < 0x10000)
		return 3;
	if (unicode < 0x200000)
		return 4;
	return 0;
}

static inline int b6_utf8_enc(unsigned int len, unsigned int unicode, void *buf)
{
	unsigned char *utf8 = buf;
	switch (len) {
	case 1:
		*utf8++ = unicode;
		break;
	case 2:
		*utf8++ = 0xc0 | (unicode >> 6);
		*utf8++ = 0x80 | (unicode & 0x3f);
		break;
	case 3:
		if (unicode >= 0xd800 && unicode < 0xe000)
			return -1;
		if (unicode >= 0xfdd0 && unicode < 0xfdf0)
			return -1;
		if (unicode >= 0xfffe)
			return -1;
		*utf8++ = 0xe0 | (unicode >> 12);
		*utf8++ = 0x80 | ((unicode >> 6) & 0x3f);
		*utf8++ = 0x80 | (unicode & 0x3f);
		break;
	case 4:
		if (unicode >= 0x1fffe && unicode < 0x20000)
			return -1;
		if (unicode >= 0x2fffe && unicode < 0xe0000)
			return -1;
		if (unicode >= 0xefffe && unicode < 0xf0000)
			return -1;
		if (unicode >= 0xffffe && unicode < 0x100000)
			return -1;
		if (unicode >= 0x10fffe)
			return -1;
		*utf8++ = 0xf0 | (unicode >> 18);
		*utf8++ = 0x80 | ((unicode >> 12) & 0x3f);
		*utf8++ = 0x80 | ((unicode >> 6) & 0x3f);
		*utf8++ = 0x80 | (unicode & 0x3f);
		break;
	default:
		return -1;
	}
	return len;
}

static inline unsigned int b6_utf8_dec_len(const void *buf)
{
	unsigned int c = *(const unsigned char*)buf;
	if (!(c & 0x80))
		return 1;
	if ((c & 0xe0) == 0xc0)
		return 2;
	if ((c & 0xf0) == 0xe0)
		return 3;
	if ((c & 0xf8) == 0xf0)
		return 4;
	return 0;
}

static inline int b6_utf8_dec(unsigned int len, unsigned int *unicode,
			      const void *buf)
{
	const unsigned char *utf8 = buf;
	unsigned int c = *utf8++;
	*unicode = c;
	switch (len) {
	case 1:
		return 1;
	case 2:
		if (c == 0xc0 || c == 0xc1)
			return -1;
		*unicode &= 0x1f;
		if (((c = *utf8++) & 0xc0) != 0x80)
			return -1;
		*unicode <<= 6;
		*unicode |= c & 0x3f;
		return 2;
	case 3:
		*unicode &= 0x0f;
		if (c == 0xe0) {
			c = *utf8++;
			if (c < 0xa0 || c > 0xbf)
				return -1;
		} else if (c == 0xed) {
			c = *utf8++;
			if (c < 0x80 || c > 0x9f)
				return -1;
		} else if (((c = *utf8++) & 0xc0) != 0x80)
			return -1;
		*unicode <<= 6;
		*unicode |= c & 0x3f;
		if (((c = *utf8++) & 0xc0) != 0x80)
			return -1;
		*unicode <<= 6;
		*unicode |= c & 0x3f;
		return 3;
	case 4:
		*unicode &= 0x07;
		if (c == 0xf0) {
			c = *utf8++;
			if (c < 0x90 || c > 0xbf)
				return -1;
		} else if (c < 0xf4) {
			if (((c = *utf8++) & 0xc0) != 0x80)
				return -1;
		} else if (c == 0xf4) {
			c = *utf8++;
			if (c < 0x80 || c > 0x8f)
				return -1;
		} else
			return -1;
		*unicode <<= 6;
		*unicode |= c & 0x3f;
		if (((c = *utf8++) & 0xc0) != 0x80)
			return -1;
		*unicode <<= 6;
		*unicode |= c & 0x3f;
		if (((c = *utf8++) & 0xc0) != 0x80)
			return -1;
		*unicode <<= 6;
		*unicode |= c & 0x3f;
		return 4;
	default:
		return -1;
	}
}

struct b6_utf8_iterator {
	const struct b6_utf8 *utf8;
	const char *ptr;
	unsigned int nchars;
};

struct b6_utf8 {
	const char *ptr;
	unsigned int nbytes;
	unsigned int nchars;
};

struct b6_utf8_string {
	struct b6_utf8 utf8;
	struct b6_allocator *allocator;
	unsigned int capacity;
};

extern const struct b6_utf8 b6_utf8_char[128];

#define B6_DEFINE_UTF8(ascii) { \
	.ptr = (ascii), \
	.nbytes = sizeof(ascii) - 1, \
	.nchars = sizeof(ascii) - 1, \
}

#define B6_UTF8(ascii) ({ \
	static const struct b6_utf8 utf8 = B6_DEFINE_UTF8(ascii); \
	&utf8; \
})

#define B6_CLONE_UTF8(utf8) { \
	.ptr = (utf8)->ptr, \
	.nbytes = (utf8)->nbytes, \
	.nchars = (utf8)->nchars, \
}

extern struct b6_utf8 *b6_utf8_from_ascii(struct b6_utf8 *self,
					  const char *asciiz);

static inline struct b6_utf8 *b6_clone_utf8(struct b6_utf8 *self,
					    const struct b6_utf8 *utf8)
{
	self->ptr = utf8->ptr;
	self->nbytes = utf8->nbytes;
	self->nchars = utf8->nchars;
	return self;
}

extern struct b6_utf8 *b6_setup_utf8(struct b6_utf8 *self,
				     const void *utf8_data,
				     unsigned int utf8_size);

static inline int b6_utf8_is_empty(const struct b6_utf8* self)
{
	return !self->nchars;
}

static inline void b6_swap_utf8(struct b6_utf8 *self, struct b6_utf8 *utf8)
{
	const char *ptr = self->ptr;
	unsigned int nbytes = self->nbytes;
	unsigned int nchars = self->nchars;
	self->ptr = utf8->ptr;
	self->nbytes = utf8->nbytes;
	self->nchars = utf8->nchars;
	utf8->ptr = ptr;
	utf8->nbytes = nbytes;
	utf8->nchars = nchars;
}

static inline void b6_utf8_to(struct b6_utf8 *self,
			      const struct b6_utf8_iterator *iter)
{
	self->ptr = iter->utf8->ptr;
	self->nchars = iter->utf8->nchars - iter->nchars;
	self->nbytes = iter->ptr - iter->utf8->ptr;
}

static inline void b6_utf8_from(struct b6_utf8 *self,
				const struct b6_utf8_iterator *iter)
{
	self->ptr = iter->ptr;
	self->nchars = iter->nchars;
	self->nbytes = iter->utf8->nbytes - (iter->ptr - iter->utf8->ptr);
}

static inline struct b6_utf8_iterator *b6_clone_utf8_iterator(
	struct b6_utf8_iterator *self, struct b6_utf8_iterator *iter)
{
	self->utf8= iter->utf8;
	self->ptr = iter->ptr;
	self->nchars = iter->nchars;
	return self;
}

static inline struct b6_utf8_iterator *b6_setup_utf8_iterator(
	struct b6_utf8_iterator *self, const struct b6_utf8 *utf8)
{
	self->utf8 = utf8;
	self->ptr = utf8->ptr;
	self->nchars = utf8->nchars;
	return self;
}

static inline int b6_utf8_iterator_has_next(const struct b6_utf8_iterator *self)
{
	return !!self->nchars;
}

static inline unsigned int b6_utf8_iterator_get_next(
	struct b6_utf8_iterator *self)
{
	unsigned int len = b6_utf8_dec_len(self->ptr);
	unsigned int unicode;
	if (len <= 0 || b6_utf8_dec(len, &unicode, self->ptr) <= 0)
		return -1;
	self->ptr += len;
	self->nchars -= 1;
	return unicode;
}

static inline struct b6_utf8_string *b6_initialize_utf8_string(
	struct b6_utf8_string *self, struct b6_allocator *allocator)
{
	self->allocator = allocator;
	self->capacity = 0;
	self->utf8.ptr = NULL;
	self->utf8.nbytes = self->utf8.nchars = 0;
	return self;
}

static inline void b6_finalize_utf8_string(struct b6_utf8_string *self)
{
	b6_deallocate(self->allocator, (void*)self->utf8.ptr);
}

static inline void b6_swap_utf8_string(struct b6_utf8_string *self,
				       struct b6_utf8_string *utf8_string)
{
	struct b6_allocator *allocator = self->allocator;
	unsigned int capacity = self->capacity;
	self->allocator = utf8_string->allocator;
	self->capacity = utf8_string->capacity;
	utf8_string->allocator = allocator;
	utf8_string->capacity = capacity;
	b6_swap_utf8(&self->utf8, &utf8_string->utf8);
}

static inline void b6_clear_utf8_string(struct b6_utf8_string *self)
{
	struct b6_utf8_string temp;
	b6_initialize_utf8_string(&temp, self->allocator);
	b6_swap_utf8_string(&temp, self);
	b6_finalize_utf8_string(&temp);
}

extern int b6_extend_utf8_string(struct b6_utf8_string *self,
				 const struct b6_utf8 *utf8);

static inline int b6_append_utf8_string(struct b6_utf8_string *self,
					unsigned int unicode)
{
	unsigned int len;
	unsigned char buf[4];
	struct b6_utf8 utf8;
	if (!(len = b6_utf8_enc_len(unicode)))
		return -3;
	if (b6_utf8_enc(len, unicode, buf) != len)
		return -2;
	return b6_extend_utf8_string(self, b6_setup_utf8(&utf8, buf, len));
}

#endif /* B6_UTF8_H */
