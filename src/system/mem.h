/*
* Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
*
* Author:      maojj <maojunjie@uniontech.com>
* Maintainer:  maojj <maojunjie@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef MEM_H
#define MEM_H

#include "private/mem_p.h"

#include <QSharedDataPointer>

#include <memory>

namespace core {
namespace system {

struct mem_stat_t {
    qulonglong memUsed {};
    qulonglong memTotal {};
    qulonglong swapUsed {};
    qulonglong swapTotal {};
};
using MemStat = std::shared_ptr<mem_stat_t>;

class MemInfo
{
public:
    explicit MemInfo();
    MemInfo(const MemInfo &other);
    MemInfo &operator=(const MemInfo &rhs);
    ~MemInfo();

    qulonglong memTotal() const;
    qulonglong memFree() const;
    qulonglong memAvailable() const;
    qulonglong buffers() const;
    qulonglong cached() const;
    qulonglong active() const;
    qulonglong inactive() const;

    qulonglong swapTotal() const;
    qulonglong swapFree() const;
    qulonglong swapCached() const;
    qulonglong shmem() const;
    qulonglong slab() const;
    qulonglong dirty() const;
    qulonglong mapped() const;

    void readMemInfo();

private:
    QSharedDataPointer<MemInfoPrivate> d;
};

inline qulonglong MemInfo::memTotal() const
{
    return d->mem_total_kb;
}

inline qulonglong MemInfo::memFree() const
{
    return d->mem_free_kb;
}

inline qulonglong MemInfo::memAvailable() const
{
    return d->mem_avail_kb;
}

inline qulonglong MemInfo::buffers() const
{
    return d->buffers_kb;
}

inline qulonglong MemInfo::cached() const
{
    return d->cached_kb;
}

inline qulonglong MemInfo::active() const
{
    return d->active_kb;
}

inline qulonglong MemInfo::inactive() const
{
    return d->inactive_kb;
}

inline qulonglong MemInfo::swapTotal() const
{
    return d->swap_total_kb;
}

inline qulonglong MemInfo::swapFree() const
{
    return d->swap_free_kb;
}

inline qulonglong MemInfo::swapCached() const
{
    return d->swap_cached_kb;
}

inline qulonglong MemInfo::shmem() const
{
    return d->shmem_kb;
}

inline qulonglong MemInfo::slab() const
{
    return d->slab_kb;
}

inline qulonglong MemInfo::dirty() const
{
    return d->dirty_kb;
}

inline qulonglong MemInfo::mapped() const
{
    return d->mapped_kb;
}

} // namespace system
} // namespace core

#endif // MEM_H
