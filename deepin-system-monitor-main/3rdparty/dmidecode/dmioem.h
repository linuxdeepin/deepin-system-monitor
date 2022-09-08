// Copyright (C) 2007-2008 Jean Delvare <jdelvare@suse.de>
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

struct dmi_header;

void dmi_set_vendor(const char *s, const char *p);
int dmi_decode_oem(const struct dmi_header *h);
