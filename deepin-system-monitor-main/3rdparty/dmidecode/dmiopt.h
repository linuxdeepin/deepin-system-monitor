// Copyright (C) 2005-2008 Jean Delvare <jdelvare@suse.de>
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "types.h"

struct string_keyword
{
	const char *keyword;
	u8 type;
	u8 offset;
};

struct opt
{
	const char *devmem;
	unsigned int flags;
	u8 *type;
	const struct string_keyword *string;
	char *dumpfile;
	u32 handle;
};
extern struct opt opt;

#define FLAG_VERSION            (1 << 0)
#define FLAG_HELP               (1 << 1)
#define FLAG_DUMP               (1 << 2)
#define FLAG_QUIET              (1 << 3)
#define FLAG_DUMP_BIN           (1 << 4)
#define FLAG_FROM_DUMP          (1 << 5)
#define FLAG_NO_SYSFS           (1 << 6)

int parse_command_line(int argc, char * const argv[]);
void print_help(void);
