#include "b6/cmdline.h"

#include "b6/registry.h"

#define _STRCMP(_lhs, _rhs, _cnv) \
	const char *__lhs = _lhs; \
	const char *__rhs = _rhs; \
	char l, r; \
	do { \
		l = _cnv(*__lhs++); \
		r = _cnv(*__rhs++); \
		if (l < r) \
			return -1; \
		if (l > r) \
			return 1; \
	} while (l && r); \
	return 0

static char *b6_strchr(const char *s, int c)
{
	do if (*s == c) return (char*) s; while (*s++); return (char*) 0;
}

static int b6_strcmp(const char *lhs, const char *rhs)
{
	_STRCMP(lhs, rhs, );
}

static int b6_tolower(int c)
{
	return (c >= 'A' && c <= 'Z') ? c |= 32 : c;
}

/*
static int b6_toupper(int c)
{
	return (c >= 'a' && c <= 'z') ? c &= ~32 : c;
}
*/

static int b6_strcasecmp(const char *lhs, const char *rhs)
{
	_STRCMP(lhs, rhs, b6_tolower);
}

static int _b6_strtoul(long unsigned int *n, const char *s, unsigned int base)
{
	long unsigned int num = 0;
	if (!base) {
		if (*s == '0') {
			if ((*++s | 32) == 'x') {
				base = 16;
				if (!*++s)
					goto parse_error;
			} else
				base = 8;
		} else
			base = 10;
	}
	for (;;) {
		char c = *s++;
		long unsigned int val;
		long unsigned int bak;
		if (c >= '0' && c <= '9')
			val = c - '0';
		else if (c >= 'a' && c <= 'f')
			val = c - 'a' + 10;
		else if (c >= 'A' && c <= 'F')
			val = c - 'A' + 10;
		else if (!c) {
			*n = num;
			return 0;
		} else
			break;
		if (val >= base)
			break;
		bak = num;
		num *= base;
		if (num < bak)
			return -2; /* overflow */
		num += val;
	}
parse_error:
	return -1;
}

static int b6_strtoul(long unsigned int *ptr, const char *str,
		      unsigned int base)
{
	if (*str == '+')
		str += 1;
	return _b6_strtoul(ptr, str, base);
}

static int b6_strtol(long int *ptr, const char *str, unsigned int base)
{
	long unsigned int num;
	int retval;
	if (*str == '-') {
		if (!(retval = _b6_strtoul(&num, str + 1, base)))
			*ptr = -num;
	} else if (!(retval = b6_strtoul(&num, str, base))) {
		if ((long int)num < 0)
			return -2;
		*ptr = num;
	}
	return retval;
}

static int b6_parse_flag(struct b6_flag *flag, const char *value)
{
	if (flag->ops != &b6_bool_flag_ops && !value)
		return -1;
	return flag->ops->parse(flag, value);
}

static char *dash_to_underscore(char *s)
{
	char *ptr;
	for (ptr = s; *ptr; ptr += 1)
		if (*ptr == '-')
			*ptr = '_';
	return s;
}

B6_REGISTRY_DEFINE(b6_flag_registry);

static struct b6_flag *b6_lookup_flag(const char *name)
{
	struct b6_entry *entry = b6_lookup_registry(&b6_flag_registry, name);
	return entry ? b6_cast_of(entry, struct b6_flag, entry) : NULL;
}

int b6_parse_command_line_flags(int argc, char *argv[], int strict)
{
	int argn, argf;
	for (argn = argf = 1; argn < argc; argn += 1) {
		char *name = argv[argn];
		char *value;
		struct b6_flag *flag;
		int i;
		if (*name++ != '-' || *name++ != '-')
			continue;
		if (*name == '\0')
			break;
		for (i = argn; i > argf; i -= 1)
			argv[i] = argv[i - 1];
		argv[argf] = name;
		if ((value = b6_strchr(name, '=')))
			*value++ = '\0';
		if ((flag = b6_lookup_flag(name)))
			b6_parse_flag(flag, value);
		else if ((flag = b6_lookup_flag(dash_to_underscore(name))))
			b6_parse_flag(flag, value);
		else if (strict)
			return -argf;
		argf += 1;
	}
	return argf;
}

#define INT_OPS(type) \
	static int b6_parse_ ## type ## _flag(struct b6_flag *flag, \
					      const char* value) \
	{ \
		long int val; \
	       	int retval = b6_strtol(&val, value, 0); \
		if (retval) \
			return retval; \
		if ((type)val != val) \
			return -2; \
		*(type*)flag->ptr = val; \
		return 0; \
	} \
	const struct b6_flag_ops b6_ ## type ## _flag_ops = { \
		.parse = b6_parse_ ## type ## _flag, \
	}; \
	static int b6_parse_u ## type ## _flag(struct b6_flag *flag, \
					       const char* value) \
	{ \
		long unsigned int val; \
	       	int retval = b6_strtoul(&val, value, 0); \
		if (retval) \
			return retval; \
		if ((type)val != val) \
			return -2; \
		*(unsigned type*)flag->ptr = val; \
		return 0; \
	} \
	const struct b6_flag_ops b6_u ## type ## _flag_ops = { \
		.parse = b6_parse_u ## type ## _flag, \
	}

INT_OPS(short);
INT_OPS(int);
INT_OPS(long);

static int b6_parse_bool_flag(struct b6_flag *flag, const char *value)
{
	int *ptr = flag->ptr;
	if (!value ||
	    !b6_strcmp(value, "1") ||
	    !b6_strcasecmp(value, "y") ||
	    !b6_strcasecmp(value, "on") ||
	    !b6_strcasecmp(value, "yes") ||
	    !b6_strcasecmp(value, "true")) {
		*ptr = 1;
		return 0;
	}
	if (!b6_strcmp(value, "0") ||
	    !b6_strcasecmp(value, "n") ||
	    !b6_strcasecmp(value, "off") ||
	    !b6_strcasecmp(value, "no") ||
	    !b6_strcasecmp(value, "false")) {
		*ptr = 0;
		return 0;
	}
	return -1;
}

const struct b6_flag_ops b6_bool_flag_ops = {
	.parse = b6_parse_bool_flag,
};

static int b6_parse_string_flag(struct b6_flag* flag, const char *value)
{
	*(const char**)flag->ptr = value;
	return 0;
}

const struct b6_flag_ops b6_string_flag_ops = {
	.parse = b6_parse_string_flag,
};

B6_REGISTRY_DEFINE(b6_cmd_registry);
