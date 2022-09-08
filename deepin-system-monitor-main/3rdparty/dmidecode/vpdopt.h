// Copyright (C) 2005-2006 Jean Delvare <jdelvare@suse.de>
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <sys/types.h>

struct string_keyword
{
	const char *keyword;
	off_t offset;
	size_t len;
};

struct opt
{
	const char *devmem;
	unsigned int flags;
	const struct string_keyword *string;
};
extern struct opt opt;

#define FLAG_VERSION            (1 << 0)
#define FLAG_HELP               (1 << 1)
#define FLAG_DUMP               (1 << 2)
#define FLAG_QUIET              (1 << 3)

int parse_command_line(int argc, char * const argv[]);
void print_help(void);
