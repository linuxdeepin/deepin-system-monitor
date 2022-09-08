// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef UTIL_LINUX_SWAP_PROBER_H
#define UTIL_LINUX_SWAP_PROBER_H

#include <blkid.h>

blkid_probe get_swap_prober(const char *devname);

#endif /* UTIL_LINUX_SWAP_PROBER_H */

