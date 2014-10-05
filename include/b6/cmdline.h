/*
 * Copyright (c) 2014, Arnaud TROEL
 * See LICENSE file for license details.
 */

/**
 * @file cmdline.h
 * @brief Define and parse command line.
 */

#ifndef CMDLINE_H
#define CMDLINE_H

#include "b6/registry.h"
#include "b6/utils.h"

/**
 * @brief Give access to a global variable via a command line flag.
 *
 * The following types are supported: `bool`, `short`, `ushort`, `int`, `uint`,
 * `long`, `ulong` and `string`.
 *
 * Flags can be defined like this:
 *
 * @code
 * static int answer_to_life_the_universe_and_everything = 0;
 * b6_flag(answer_to_life_the_universe_and_everything, int);
 * @endcode
 *
 * Command line flags start with a double dash and accept a value after an equal
 * sign. The flag has the same name as the variable it refers to. Underscores
 * are converted to dashes.
 *
 * Example: my_hitchhiker_guide --answer-to-life-the-universe-and-everything=42
 *
 * @param _var specifies the variable.
 * @param _type specifies its type.
 */
#define b6_flag(_var, _type) b6_flag_named(_var, _type, #_var)

/**
 * @brief Give access to a global variable via a command line flag alias.
 *
 * Flags can be aliased as follows:
 *
 * @code
 * static int answer_to_life_the_universe_and_everything = 0;
 * b6_flag(answer_to_life_the_universe_and_everything, int, answer_to_life);
 * @endcode
 *
 * Example: my_hitchhiker_guide --answer-to-life=42
 *
 * @param _var specifies the variable.
 * @param _type specifies its type.
 * @param _name specifies the name of the flag.
 *
 * @see b6_flag
 */
#define b6_flag_named(_var, _type, _name) \
	static struct b6_flag flag_ ## _var; \
	b6_ctor(b6_flag_initialize_ ## _var); \
	static void b6_flag_initialize_ ## _var(void) \
	{ \
		struct b6_flag *flag = &flag_ ## _var; \
		flag->ops = &b6_ ## _type ## _flag_ops; \
		flag->ptr = &_var; \
		b6_register(&b6_flag_registry, &flag->entry, _name); \
		(void) ((b6_flag_type_ ## _type*)0 == &_var); \
	} \
	static struct b6_flag flag_ ## _var

/**
 * @brief Parse command line flags.
 *
 * Command line arguments order is preserved. Flags are grouped and moved at the
 * beginning of the command line. Non-flags follow.
 *
 * @param argc specifies the number of arguments of the command line.
 * @param argv specifies the arguments of the command line.
 * @param strict specifies if the parsing should fail on unknown flags
 * @return the index of the first command line argument that is not a flag.
 * @return the negative index of the first flag that was not recognized (for
 * strict parsing)
 */
extern int b6_parse_command_line_flags(int argc, char *argv[], int strict);

struct b6_flag {
	struct b6_entry entry;
	const struct b6_flag_ops *ops;
	void *ptr;
};

extern struct b6_registry b6_flag_registry;

struct b6_flag_ops {
	int (*parse)(struct b6_flag*, const char*);
};

#define b6_declare_flag_type(_alias, _type) \
	typedef _type b6_flag_type_ ## _alias; \
	extern const struct b6_flag_ops b6_ ## _alias ## _flag_ops

b6_declare_flag_type(bool, int);
b6_declare_flag_type(short, short);
b6_declare_flag_type(ushort, short unsigned);
b6_declare_flag_type(int, int);
b6_declare_flag_type(uint, unsigned);
b6_declare_flag_type(long, long);
b6_declare_flag_type(ulong, long unsigned);
b6_declare_flag_type(string, const char*);

#undef b6_declare_flag_type

struct b6_cmd {
	struct b6_entry entry;
	const struct b6_cmd_ops *ops;
};

extern struct b6_registry b6_cmd_registry;

struct b6_cmd_ops {
	int (*exec)(struct b6_cmd*, int argc, char *argv[]);
};

#define b6_cmd(_fun) \
	static const struct b6_cmd_ops _fun##_ops = { .exec = _fun, }; \
	static struct b6_cmd _fun##_cmd = { .ops = &(_fun##_ops), }; \
	b6_ctor(b6_cmd_register_##_fun); \
	static void b6_cmd_register_##_fun(void) \
	{ \
		b6_register(&b6_cmd_registry, &(_fun##_cmd).entry, #_fun); \
	} \
	static void b6_cmd_register_##_fun(void)

static inline struct b6_cmd *b6_lookup_cmd(const char *name)
{
	const struct b6_entry *e = b6_lookup_registry(&b6_cmd_registry, name);
	return e ? b6_cast_of(e, struct b6_cmd, entry) : NULL;
}

static inline int b6_exec_cmd(struct b6_cmd *self, int argc, char *argv[])
{
	return self->ops->exec(self, argc, argv);
}

#endif /* CMDLINE_H */
