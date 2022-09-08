// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SIGNAMES_H
#define SIGNAMES_H

int signame_to_signum(const char *sig);
const char *signum_to_signame(int signum);
int get_signame_by_idx(size_t idx, const char **signame, int *signum);

#endif /* SIGNAMES_H */
