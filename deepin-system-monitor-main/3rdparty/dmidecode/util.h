// Copyright (C) 2003-2017 Jean Delvare <jdelvare@suse.de>
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <sys/types.h>

#include "types.h"

#define ARRAY_SIZE(x) (sizeof(x)/sizeof((x)[0]))

int checksum(const u8 *buf, size_t len);
void *read_file(off_t base, size_t *len, const char *filename);
void *mem_chunk(off_t base, size_t len, const char *devmem);
int write_dump(size_t base, size_t len, const void *data, const char *dumpfile, int add);
u64 u64_range(u64 start, u64 end);
