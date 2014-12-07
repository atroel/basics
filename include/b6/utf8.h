#ifndef B6_UTF8_H
#define B6_UTF8_H

static inline int b6_utf8_enc_len(unsigned int unicode)
{
	if (unicode < 0x80)
		return 1;
	if (unicode < 0x800)
		return 2;
	if (unicode < 0x10000)
		return 3;
	if (unicode < 0x200000)
		return 4;
	return -1;
}

static inline int b6_utf8_enc(int len, unsigned int unicode, void *buf)
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

static inline int b6_utf8_dec_len(const void *buf)
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
	return -1;
}

static inline int b6_utf8_dec(int len, unsigned int *unicode, const void *buf)
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

#endif /* B6_UTF8_H */
