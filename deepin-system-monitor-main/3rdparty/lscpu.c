// Copyright (C) 2008 Cai Qian <qcai@redhat.com>
// Copyright (C) 2008 Karel Zak <kzak@redhat.com>
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <assert.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/utsname.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/personality.h>

#include <libsmartcols.h>

#include "closestream.h"
#include "optutils.h"

#include "lscpu.h"

struct polarization_modes {
	char *parsable;
	char *readable;
};


/*
 * IDs
 */
enum {
	COL_CPU_BOGOMIPS,
	COL_CPU_CPU,
	COL_CPU_CORE,
	COL_CPU_SOCKET,
	COL_CPU_CLUSTER,
	COL_CPU_NODE,
	COL_CPU_BOOK,
	COL_CPU_DRAWER,
	COL_CPU_CACHE,
	COL_CPU_POLARIZATION,
	COL_CPU_ADDRESS,
	COL_CPU_CONFIGURED,
	COL_CPU_ONLINE,
	COL_CPU_MHZ,
	COL_CPU_SCALMHZ,
	COL_CPU_MAXMHZ,
	COL_CPU_MINMHZ,
};

enum {
	COL_CACHE_ALLSIZE,
	COL_CACHE_LEVEL,
	COL_CACHE_NAME,
	COL_CACHE_ONESIZE,
	COL_CACHE_TYPE,
	COL_CACHE_WAYS,
	COL_CACHE_ALLOCPOL,
	COL_CACHE_WRITEPOL,
	COL_CACHE_PHYLINE,
	COL_CACHE_SETS,
	COL_CACHE_COHERENCYSIZE
};


/* column description
 */
struct lscpu_coldesc {
	const char *name;
	const char *help;

	int flags;
	unsigned int  is_abbr:1;	/* name is abbreviation */
	int json_type;
};

UL_DEBUG_DEFINE_MASK(lscpu);
