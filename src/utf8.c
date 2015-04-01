/*
 * Copyright (c) 2009-2015, Arnaud TROEL
 * See LICENSE file for license details.
 */

#include "b6/utf8.h"

static const char ascii[] =
	"\001\002\003\004\005\006\007\010\011\012\013\014\015\016\017\020"
	"\021\022\023\024\025\026\027\030\031\032\033\034\035\036\037"
	" !\"#$%&\'()*+,-./0123456789:;<=>?"
	"@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_"
	"`abcdefghijklmnopqrstuvwxyz{|}~\177";


#define B6_UTF8_CHR(i) { \
	.ptr = &ascii[(i + 127)&127], .nbytes = 1, .nchars = 1, \
}

const struct b6_utf8 b6_utf8_char[128] = {
	B6_UTF8_CHR(  0), B6_UTF8_CHR(  1), B6_UTF8_CHR(  2), B6_UTF8_CHR(  3),
	B6_UTF8_CHR(  4), B6_UTF8_CHR(  5), B6_UTF8_CHR(  6), B6_UTF8_CHR(  7),
	B6_UTF8_CHR(  8), B6_UTF8_CHR(  9), B6_UTF8_CHR( 10), B6_UTF8_CHR( 11),
	B6_UTF8_CHR( 12), B6_UTF8_CHR( 13), B6_UTF8_CHR( 14), B6_UTF8_CHR( 15),
	B6_UTF8_CHR( 16), B6_UTF8_CHR( 17), B6_UTF8_CHR( 18), B6_UTF8_CHR( 19),
	B6_UTF8_CHR( 20), B6_UTF8_CHR( 21), B6_UTF8_CHR( 22), B6_UTF8_CHR( 23),
	B6_UTF8_CHR( 24), B6_UTF8_CHR( 25), B6_UTF8_CHR( 26), B6_UTF8_CHR( 27),
	B6_UTF8_CHR( 28), B6_UTF8_CHR( 29), B6_UTF8_CHR( 30), B6_UTF8_CHR( 31),
	B6_UTF8_CHR( 32), B6_UTF8_CHR( 33), B6_UTF8_CHR( 34), B6_UTF8_CHR( 35),
	B6_UTF8_CHR( 36), B6_UTF8_CHR( 37), B6_UTF8_CHR( 38), B6_UTF8_CHR( 39),
	B6_UTF8_CHR( 40), B6_UTF8_CHR( 41), B6_UTF8_CHR( 42), B6_UTF8_CHR( 43),
	B6_UTF8_CHR( 44), B6_UTF8_CHR( 45), B6_UTF8_CHR( 46), B6_UTF8_CHR( 47),
	B6_UTF8_CHR( 48), B6_UTF8_CHR( 49), B6_UTF8_CHR( 50), B6_UTF8_CHR( 51),
	B6_UTF8_CHR( 52), B6_UTF8_CHR( 53), B6_UTF8_CHR( 54), B6_UTF8_CHR( 55),
	B6_UTF8_CHR( 56), B6_UTF8_CHR( 57), B6_UTF8_CHR( 58), B6_UTF8_CHR( 59),
	B6_UTF8_CHR( 60), B6_UTF8_CHR( 61), B6_UTF8_CHR( 62), B6_UTF8_CHR( 63),
	B6_UTF8_CHR( 64), B6_UTF8_CHR( 65), B6_UTF8_CHR( 66), B6_UTF8_CHR( 67),
	B6_UTF8_CHR( 68), B6_UTF8_CHR( 69), B6_UTF8_CHR( 70), B6_UTF8_CHR( 71),
	B6_UTF8_CHR( 72), B6_UTF8_CHR( 73), B6_UTF8_CHR( 74), B6_UTF8_CHR( 75),
	B6_UTF8_CHR( 76), B6_UTF8_CHR( 77), B6_UTF8_CHR( 78), B6_UTF8_CHR( 79),
	B6_UTF8_CHR( 80), B6_UTF8_CHR( 81), B6_UTF8_CHR( 82), B6_UTF8_CHR( 83),
	B6_UTF8_CHR( 84), B6_UTF8_CHR( 85), B6_UTF8_CHR( 86), B6_UTF8_CHR( 87),
	B6_UTF8_CHR( 88), B6_UTF8_CHR( 89), B6_UTF8_CHR( 90), B6_UTF8_CHR( 91),
	B6_UTF8_CHR( 92), B6_UTF8_CHR( 93), B6_UTF8_CHR( 94), B6_UTF8_CHR( 95),
	B6_UTF8_CHR( 96), B6_UTF8_CHR( 97), B6_UTF8_CHR( 98), B6_UTF8_CHR( 99),
	B6_UTF8_CHR(100), B6_UTF8_CHR(101), B6_UTF8_CHR(102), B6_UTF8_CHR(103),
	B6_UTF8_CHR(104), B6_UTF8_CHR(105), B6_UTF8_CHR(106), B6_UTF8_CHR(107),
	B6_UTF8_CHR(108), B6_UTF8_CHR(109), B6_UTF8_CHR(110), B6_UTF8_CHR(111),
	B6_UTF8_CHR(112), B6_UTF8_CHR(113), B6_UTF8_CHR(114), B6_UTF8_CHR(115),
	B6_UTF8_CHR(116), B6_UTF8_CHR(117), B6_UTF8_CHR(118), B6_UTF8_CHR(119),
	B6_UTF8_CHR(120), B6_UTF8_CHR(121), B6_UTF8_CHR(122), B6_UTF8_CHR(123),
	B6_UTF8_CHR(124), B6_UTF8_CHR(125), B6_UTF8_CHR(126), B6_UTF8_CHR(127),
};

struct b6_utf8 *b6_utf8_from_ascii(struct b6_utf8 *self, const char *ptr)
{
	self->ptr = ptr;
	for (self->nchars = 0; *ptr++; self->nchars += 1);
	self->nbytes = self->nchars;
	return self;
}

struct b6_utf8 *b6_setup_utf8(struct b6_utf8 *self, const void *utf8_data,
			      unsigned int utf8_size)
{
	const unsigned char *ptr = utf8_data;
	self->ptr = utf8_data;
	self->nbytes = utf8_size;
	self->nchars = 0;
	while (utf8_size > 0) {
		int len = b6_utf8_dec_len(ptr);
		if (len <= 0 || len > utf8_size)
			break;
		ptr += len;
		utf8_size -= len;
		self->nchars += 1;
	}
	return self;
}

int b6_extend_utf8_string(struct b6_utf8_string *self,
			  const struct b6_utf8 *utf8)
{
	char *dst;
	const char *src;
	unsigned int nbytes = self->utf8.nbytes + utf8->nbytes + 1;
	if (nbytes > self->capacity) {
		unsigned int capacity;
		char *buffer;
		if (nbytes <= 2048)
			for (capacity = self->capacity ? self->capacity : 1;
			     capacity <= nbytes; capacity += capacity);
		else if (nbytes & 1023)
			capacity = (nbytes & ~1023) + 1024;
		else
			capacity = nbytes;
		if (capacity < self->capacity)
			return -1;
		if (!(buffer = b6_reallocate(self->allocator,
					     (void*)self->utf8.ptr, capacity)))
			return -1;
		self->utf8.ptr = buffer;
		self->capacity = capacity;
	}
	src = utf8->ptr;
	dst = (void*)self->utf8.ptr + self->utf8.nbytes;
	while (src < utf8->ptr + utf8->nbytes)
		*dst++ = *src++;
	*dst = '\0';
	self->utf8.nbytes += utf8->nbytes;
	self->utf8.nchars += utf8->nchars;
	return 0;
}
