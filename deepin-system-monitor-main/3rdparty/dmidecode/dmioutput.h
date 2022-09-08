// Copyright (C) 2020 Jean Delvare <jdelvare@suse.de>
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dmidecode.h"

void pr_comment(const char *format, ...);
void pr_info(const char *format, ...);
void pr_handle(const struct dmi_header *h);
void pr_handle_name(const char *format, ...);
void pr_attr(const char *name, const char *format, ...);
void pr_subattr(const char *name, const char *format, ...);
void pr_list_start(const char *name, const char *format, ...);
void pr_list_item(const char *format, ...);
void pr_list_end(void);
void pr_sep(void);
void pr_struct_err(const char *format, ...);
