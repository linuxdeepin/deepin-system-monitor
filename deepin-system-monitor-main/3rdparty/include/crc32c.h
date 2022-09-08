// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef UL_NG_CRC32C_H
#define UL_NG_CRC32C_H

#include <sys/types.h>
#include <stdint.h>

extern uint32_t crc32c(uint32_t crc, const void *buf, size_t size);

#endif /* UL_NG_CRC32C_H */
