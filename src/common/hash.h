/*
* Copyright (C) 2019 ~ 2021 Uniontech Software Technology Co.,Ltd.
*
* Author:     leiyu <leiyu@uniontech.com>
*
* Maintainer: leiyu <leiyu@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef HASH_H
#define HASH_H

#include <sys/types.h>
#include <cstdint>

namespace util {
namespace common {

// global seed
extern uint32_t global_seed;

/**
* @brief init_seed Init global seed
*/
void init_seed();

/*
 * The following hash function is based on MurmurHash3, placed into the public
 * domain by Austin Appleby.  See https://github.com/aappleby/smhasher for
 * details.
 */

void hash_x86_32(const void *key, int len, uint32_t seed, uint32_t *out);
void hash_x86_128(const void *key, int len, uint32_t seed, uint64_t out[2]);
void hash_x64_128(const void *key, int len, uint32_t seed, uint64_t out[2]);

/**
* @brief hash Generate hash code with specific input source
* @param key Input source
* @param len Source buffer length
* @param seed Seed
* @param out Output hash code array
*/
inline void hash(const void *key, int len, uint32_t seed, uint64_t out[2])
{
#if (__SIZEOF_POINTER__ == 8 && BYTE_ORDER != BIG_ENDIAN)
    hash_x64_128(key, len, seed, out);
#else
    hash_x86_128(key, len, seed, out);
#endif
}

} // namespace common
} // namespace util

#endif // HASH_H
