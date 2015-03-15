/*
 * Copyright (c) 2009-2015, Arnaud TROEL
 * See LICENSE file for license details.
 */

#include "b6/utf8.h"

const void *b6_ascii_to_utf8(const char *s, unsigned int *size)
{
	const void *utf8 = s;
	for (*size = 0; *s++; *size += 1);
	return utf8;
}
