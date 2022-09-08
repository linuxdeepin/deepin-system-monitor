// Copyright (C) 2020 Jean Delvare <jdelvare@suse.de>
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <stdarg.h>
#include <stdio.h>
#include "dmioutput.h"

void pr_comment(const char *format, ...)
{
	va_list args;

	printf("# ");
	va_start(args, format);
	vprintf(format, args);
	va_end(args);
	printf("\n");
}

void pr_info(const char *format, ...)
{
	va_list args;

	va_start(args, format);
	vprintf(format, args);
	va_end(args);
	printf("\n");
}

void pr_handle(const struct dmi_header *h)
{
	printf("Handle 0x%04X, DMI type %d, %d bytes\n",
	       h->handle, h->type, h->length);
}

void pr_handle_name(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	vprintf(format, args);
	va_end(args);
	printf("\n");
}

void pr_attr(const char *name, const char *format, ...)
{
	va_list args;

	printf("\t%s: ", name);

	va_start(args, format);
	vprintf(format, args);
	va_end(args);
	printf("\n");
}

void pr_subattr(const char *name, const char *format, ...)
{
	va_list args;

	printf("\t\t%s: ", name);

	va_start(args, format);
	vprintf(format, args);
	va_end(args);
	printf("\n");
}

void pr_list_start(const char *name, const char *format, ...)
{
	va_list args;

	printf("\t%s:", name);

	/* format is optional, skip value if not provided */
	if (format)
	{
		printf(" ");
		va_start(args, format);
		vprintf(format, args);
		va_end(args);
	}
	printf("\n");

}

void pr_list_item(const char *format, ...)
{
	va_list args;

	printf("\t\t");

	va_start(args, format);
	vprintf(format, args);
	va_end(args);
	printf("\n");
}

void pr_list_end(void)
{
	/* a no-op for text output */
}

void pr_sep(void)
{
	printf("\n");
}

void pr_struct_err(const char *format, ...)
{
	va_list args;

	printf("\t");

	va_start(args, format);
	vprintf(format, args);
	va_end(args);
	printf("\n");
}
