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

#include "cpu.h"
#include "cpu_set.h"
#include "private/cpu_p.h"

#define PATH_TPL_CPU_CACHE "/sys/devices/system/cpu/%d/cache"
#define PATH_TPL_CPU_CACHE_ID "/sys/devices/system/cpu/%d/cache/index%d/id"
#define PATH_TPL_CPU_CACHE_TYPE "/sys/devices/system/cpu/%d/cache/index%d/type"
#define PATH_TPL_CPU_CACHE_SIZE "/sys/devices/system/cpu/%d/cache/index%d/size"
#define PATH_TPL_CPU_CACHE_LEVEL "/sys/devices/system/cpu/%d/cache/index%d/level"
#define PATH_TPL_CPU_CACHE_SHR_CPU_LIST "/sys/devices/system/cpu/%d/cache/index%d/shared_cpu_list"

#define PATH_TPL_CPU_FREQ_MAX "/sys/devices/system/cpu/%d/cpufreq/cpuinfo_max_freq"
#define PATH_TPL_CPU_FREQ_MIN "/sys/devices/system/cpu/%d/cpufreq/cpuinfo_min_freq"
#define PATH_TPL_CPU_FREQ_CUR "/sys/devices/system/cpu/%d/cpufreq/scaling_cur_freq"

#define PATH_TPL_CPU_TOPO_CORE_ID "/sys/devices/system/cpu/%d/topology/core_id"
#define PATH_TPL_CPU_TOPO_CORE_SIBLINGS "/sys/devices/system/cpu/%d/topology/core_siblings_list"
#define PATH_TPL_CPU_TOPO_PHY_PKG_ID "/sys/devices/system/cpu/%d/topology/physical_package_id"
#define PATH_TPL_CPU_TOPO_THR_SIBLINGS "/sys/devices/system/cpu/%d/topology/thread_siblings_list"

namespace core {
namespace system {

CPUInfo::CPUInfo()
    : d(new CPUInfoPrivate())
{
}
CPUInfo::CPUInfo(int index)
    : d(new CPUInfoPrivate())
{
    readCPUInfo(index);
}
CPUInfo::CPUInfo(const CPUInfo &other)
    : d(other.d)
{
}
CPUInfo &CPUInfo::operator=(const CPUInfo &rhs)
{
    if (this == &rhs)
        return *this;

    d = rhs.d;
    return *this;
}
CPUInfo::~CPUInfo()
{
}

void CPUInfo::readCPUInfo(int index)
{
    read_min_freq(index);
    read_max_freq(index);
    read_cur_freq(index);
    read_cache(index);
    read_topology(index);
}

void CPUInfo::read_min_freq(int index)
{
}

void CPUInfo::read_max_freq(int index)
{
}

void CPUInfo::read_cur_freq(int index)
{
}

void CPUInfo::read_cache(int index)
{
}

void CPUInfo::read_topology(int index)
{
}

int CPUInfo::logicalIndex() const
{
    return d->index;
}

QByteArray CPUInfo::logicalName() const
{
    return d->cpu;
}

quint32 CPUInfo::curfreq() const
{
    return d->curfreq;
}

quint32 CPUInfo::minfreq() const
{
    return d->minfreq;
}

quint32 CPUInfo::maxfreq() const
{
    return d->maxfreq;
}

int CPUInfo::packageID() const
{
    return d->package_id;
}

int CPUInfo::coreID() const
{
    return d->core_id;
}

int CPUInfo::bookID() const
{
    return d->book_id;
}

int CPUInfo::drawerID() const
{
    return d->drawer_id;
}

qulonglong CPUInfo::l1iCache() const
{
    for (auto &ce : d->cache) {
        if (ce->level == 1 && ce->type == "Instruction")
            return ce->size;
    }

    return 0;
}

qulonglong CPUInfo::l1dCache() const
{
    for (auto &ce : d->cache) {
        if (ce->level == 1 && ce->type == "Data")
            return ce->size;
    }

    return 0;
}

qulonglong CPUInfo::l2Cache() const
{
    for (auto &ce : d->cache) {
        if (ce->level == 2)
            return ce->size;
    }

    return 0;
}

qulonglong CPUInfo::l3Cache() const
{
    for (auto &ce : d->cache) {
        if (ce->level == 3)
            return ce->size;
    }

    return 0;
}

} // namespace system
} // namespace core
