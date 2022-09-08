// Copyright (C) 2005-2020 Jean Delvare <jdelvare@suse.de>
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DMIDECODE_H
#define DMIDECODE_H

#include "types.h"
#ifdef __cplusplus
extern "C" {
#endif
extern  u16 MaxCPUFreq;
extern  u16 CurrentCPUFreq;
int is_printable(const u8 *data, int len);
//const char *dmidecode_dmi_string(const struct dmi_header *dm, u8 s);
void dmi_print_memory_size(const char *addr, u64 code, int shift);

int get_cpuinfo_from_dmi(int argc, char *const argv[]);
//int smbios3_decode(u8 *buf, const char *devmem, u32 flags);
int smbios_decode(u8 *buf, const char *devmem, u32 flags);
int legacy_decode(u8 *buf, const char *devmem, u32 flags);

#ifdef __cplusplus
}
#endif

struct dmi_header {
    u8 type;
    u8 length;
    u16 handle;
    u8 *data;
};
void dmi_decode(const struct dmi_header *h, u16 ver);



#endif
