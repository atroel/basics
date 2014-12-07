#include "b6/utf8.h"
#include "b6/utils.h"
#include "test.h"

#define verify(condition, format, args...) do \
	if (!(condition)) { \
		fprintf(stderr, "%s:%d: " format "\n", \
			__func__, __LINE__, args); \
		return 0; \
	} while (0)

static int encode_ascii()
{
	unsigned int u;
	unsigned char utf8[1];
	for (u = 0; u < 0x80; u += 1) {
		verify(b6_utf8_enc_len(u) == 1, "0x%02x", u);
		verify(b6_utf8_enc(sizeof(utf8), u, utf8) == 1, "0x%02x", u);
		verify(utf8[0] == u, "expected 0x%02x, got 0x%02x", u, utf8[0]);
	}
	return 1;
}

static int encode_2_bytes()
{
	const unsigned int u = 0xa2;
	unsigned char utf8[2];
	int len, ret;
	verify((len = b6_utf8_enc_len(u)) == sizeof(utf8), "%d", len);
	verify((ret = b6_utf8_enc(sizeof(utf8), u, utf8)) == len, "%d", ret);
	verify(utf8[0] == 0xc2, "expected 0xc2, got 0x%02x", utf8[0]);
	verify(utf8[1] == 0xa2, "expected 0xa2, got 0x%02x", utf8[1]);
	return 1;
}

static int encode_3_bytes()
{
	const unsigned int u = 0x20ac;
	unsigned char utf8[3];
	int len, ret;
	verify((len = b6_utf8_enc_len(u)) == sizeof(utf8), "%d", len);
	verify((ret = b6_utf8_enc(sizeof(utf8), u, utf8)) == len, "%d", ret);
	verify(utf8[0] == 0xe2, "expected 0xe2, got 0x%02x", utf8[0]);
	verify(utf8[1] == 0x82, "expected 0x82, got 0x%02x", utf8[1]);
	verify(utf8[2] == 0xac, "expected 0xac, got 0x%02x", utf8[2]);
	return 1;
}

static int encode_4_bytes()
{
	const unsigned int u = 0x24b62;
	unsigned char utf8[4];
	int len, ret;
	verify((len = b6_utf8_enc_len(u)) == sizeof(utf8), "%d", len);
	verify((ret = b6_utf8_enc(sizeof(utf8), u, utf8)) == len, "%d", ret);
	verify(utf8[0] == 0xf0, "expected 0xf0, got 0x%02x", utf8[0]);
	verify(utf8[1] == 0xa4, "expected 0xa4, got 0x%02x", utf8[1]);
	verify(utf8[2] == 0xad, "expected 0xad, got 0x%02x", utf8[2]);
	verify(utf8[3] == 0xa2, "expected 0xa2, got 0x%02x", utf8[3]);
	return 1;
}

static int encode_valid()
{
	static const struct { unsigned int lo, hi; } bounds[] = {
		{ 0x0000, 0xd800 },
		{ 0xe000, 0xfdd0 },
		{ 0xfdf0, 0xfffd },
		{ 0x10000, 0x1fffe },
		{ 0x20000, 0x2fffe },
		{ 0xe0000, 0xefffe },
		{ 0xf0000, 0xffffe },
		{ 0x100000, 0x10fffe },
	};
	int i = b6_card_of(bounds);
	while (--i) {
		unsigned int u;
		for (u = bounds[i].lo; u < bounds[i].hi; u += 1) {
			int len;
			unsigned char utf8[4];
			verify((len = b6_utf8_enc_len(u)) > 0, "0x%04x", u);
			verify(b6_utf8_enc(len, u, utf8) == len, "0x%04x", u);
		}
	}
	return 1;
}

static int encode_invalid()
{
	static const struct { unsigned int lo, hi; } bounds[] = {
		{ 0xd800, 0xe000 },
		{ 0xfdd0, 0xfdf0 },
		{ 0xfffe, 0x10000 },
		{ 0x1fffe, 0x20000 },
		{ 0x2fffe, 0xe0000 },
		{ 0xefffe, 0xf0000 },
		{ 0xffffe, 0x100000 },
		{ 0x10fffe, 0x120000 },
		{ 0x120000, 0x200000 },
	};
	int len;
	unsigned char utf8[4];
	unsigned int u, i;
	for (i = b6_card_of(bounds); --i;)
		for (u = bounds[i].lo; u < bounds[i].hi; u += 1) {
			verify((len = b6_utf8_enc_len(u)) > 0, "0x%04x", u);
			verify(b6_utf8_enc(len, u, utf8) <= 0, "0x%04x", u);
		}
	verify(b6_utf8_enc_len(0x200000) <= 0, "0x%04x", 0x200000);
	verify(b6_utf8_enc_len(0x1000000) <= 0, "0x%04x", 0x1000000);
	verify(b6_utf8_enc_len(0xffffffff) <= 0, "0x%04x", 0xffffffff);
	return 1;
}

static int decode_ascii()
{
	unsigned int u;
	unsigned char utf8[1];
	for (utf8[0] = 0; utf8[0] < 0x80; utf8[0] += 1) {
		verify(b6_utf8_dec_len(utf8) == 1, "0x%02x", utf8[0]);
		verify(b6_utf8_dec(1, &u, utf8) == 1, "0x%02x", utf8[0]);
		verify(utf8[0] == u, "expected 0x%02x got 0x%02x", utf8[0], u);
	}
	return 1;
}

static int decode_2_bytes()
{
	unsigned int u;
	const unsigned char utf8[] = { 0xc2, 0xa2 };
	int len, ret;
	verify((len = b6_utf8_dec_len(utf8)) == sizeof(utf8), "%d", len);
	verify((ret = b6_utf8_dec(sizeof(utf8), &u, utf8)) == len, "%d", ret);
	verify(u == 0xa2, "expected 0xa2 got 0x%02x", u);
	return 1;
}

static int decode_3_bytes()
{
	unsigned int u = 0x20ac;
	const unsigned char utf8[] = { 0xe2, 0x82, 0xac };
	int len, ret;
	verify((len = b6_utf8_dec_len(utf8)) == sizeof(utf8), "%d", len);
	verify((ret = b6_utf8_dec(sizeof(utf8), &u, utf8) == len), "%d", ret);
	verify(u == 0x20ac, "expected 0x20ac, got 0x%04x", u);
	return 1;
}

static int decode_4_bytes()
{
	unsigned int u = 0x24b62;
	const unsigned char utf8[] = { 0xf0, 0xa4, 0xad, 0xa2 };
	int len, ret;
	verify((len = b6_utf8_dec_len(utf8)) == sizeof(utf8), "%d", len);
	verify((ret = b6_utf8_dec(len, &u, utf8)) == len, "%d", ret);
	verify(u == 0x24b62, "expected 0x24b62, got 0x%x", utf8[0]);
	return 1;
}

static int decode_invalid1()
{
	unsigned char utf8[] = { 0xc0, 0x80 };
	unsigned int u;
	while (utf8[0] <= 0xc1) {
		verify(b6_utf8_dec(b6_utf8_dec_len(utf8), &u, utf8) <= 0,
		       "0x%02x", utf8[0]);
		utf8[0] += 1;
	}
	return 1;
}

static int decode_invalid2()
{
	unsigned char utf8[] = { 0xe0, 0x80, 0x80 };
	unsigned int u;
	while (utf8[1] <= 0x9f) {
		verify(b6_utf8_dec(b6_utf8_dec_len(utf8), &u, utf8) <= 0,
		       "0x%02x", utf8[1]);
		utf8[1] += 1;
	}
	return 1;
}

static int decode_invalid3()
{
	unsigned char utf8[] = { 0xed, 0xa0, 0x80, 0x80 };
	unsigned int u;
	while (utf8[1] <= 0xbf) {
		verify(b6_utf8_dec(b6_utf8_dec_len(utf8), &u, utf8) <= 0,
		       "0x%02x", utf8[1]);
		utf8[1] += 1;
	}
	return 1;
}

static int decode_invalid4()
{
	unsigned char utf8[] = { 0xf4, 0x90, 0x80, 0x80 };
	unsigned int u;
	while (utf8[1] <= 0xbf) {
		verify(b6_utf8_dec(b6_utf8_dec_len(utf8), &u, utf8) <= 0,
		       "0x%02x", utf8[1]);
		utf8[1] += 1;
	}
	return 1;
}

static int decode_invalid5()
{
	unsigned char utf8[] = { 0xf5, 0x80, 0x80, 0x80 };
	unsigned int u;
	while (utf8[0]) {
		verify(b6_utf8_dec(b6_utf8_dec_len(utf8), &u, utf8) <= 0,
		       "0x%02x", utf8[0]);
		utf8[0] += 1;
	}
	return 1;
}

static int decode_overlong()
{
	unsigned char utf8[] = { 0xf0, 0x82, 0x82, 0xac };
	unsigned int u;
	verify(b6_utf8_dec(b6_utf8_dec_len(utf8), &u, utf8) <= 0, "%s", "");
	return 1;
}

int main(int argc, const char *argv[])
{
	test_init();
	test_exec(encode_ascii,);
	test_exec(encode_2_bytes,);
	test_exec(encode_3_bytes,);
	test_exec(encode_4_bytes,);
	test_exec(encode_valid,);
	test_exec(encode_invalid,);
	test_exec(decode_ascii,);
	test_exec(decode_2_bytes,);
	test_exec(decode_3_bytes,);
	test_exec(decode_4_bytes,);
	test_exec(decode_invalid1,);
	test_exec(decode_invalid2,);
	test_exec(decode_invalid3,);
	test_exec(decode_invalid4,);
	test_exec(decode_invalid5,);
	test_exec(decode_overlong,);
	test_exit();
	return 0;
}
