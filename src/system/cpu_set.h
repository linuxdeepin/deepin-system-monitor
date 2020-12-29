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
#ifndef CPUSET_H
#define CPUSET_H

#include "cpu.h"
#include "private/cpu_set_p.h"

#include <QList>
#include <QSharedDataPointer>

namespace core {
namespace system {

class CPUSet
{
public:
    CPUSet();
    CPUSet(const CPUSet &other);
    CPUSet &operator=(const CPUSet &rhs);
    ~CPUSet();

    quint32 curfreq() const;
    quint32 minfreq() const;
    quint32 maxfreq() const;
    QString cpuModel() const;
    QString cpuVendor() const;
    quint32 cores() const;
    quint32 sockets() const;
    quint32 processors() const;
    QString virtualization() const;
    qulonglong l1iCache() const;
    qulonglong l1dCache() const;
    qulonglong l2Cache() const;
    qulonglong l3Cache() const;

    // overall usage & stat
    const CPUUsage usage() const;
    const CPUStat stat() const;

    QList<CPUInfo> cpuInfoDB() const;
    // per cpu usage & stats
    const CPUStat statDB(const QByteArray &cpu) const;
    const CPUUsage usageDB(const QByteArray &cpu) const;

protected:
    void update();

private:
    void read_stats();
    void read_overall_info();

private:
    QSharedDataPointer<CPUSetPrivate> d;
};

inline quint32 CPUSet::curfreq() const
{
    quint32 freq = 0;
    for (auto &cpuInfo : d->m_cpuInfoDB) {
        if (freq < cpuInfo.curfreq())
            freq = cpuInfo.curfreq();
    }

    return freq;
}

inline quint32 CPUSet::minfreq() const
{
    quint32 freq = UINT32_MAX;
    for (auto &cpuInfo : d->m_cpuInfoDB) {
        if (freq > cpuInfo.minfreq())
            freq = cpuInfo.minfreq();
    }

    return freq;
}

inline quint32 CPUSet::maxfreq() const
{
    quint32 freq = 0;
    for (auto &cpuInfo : d->m_cpuInfoDB) {
        if (freq < cpuInfo.maxfreq())
            freq = cpuInfo.maxfreq();
    }

    return freq;
}

inline QString CPUSet::cpuModel() const
{
    return d->m_model;
}

inline QString CPUSet::cpuVendor() const
{
    return d->m_vendor;
}

inline quint32 CPUSet::cores() const
{
    return d->m_cores;
}

inline quint32 CPUSet::sockets() const
{
    return d->m_sockets;
}

inline quint32 CPUSet::processors() const
{
    return d->m_processors;
}

inline QString CPUSet::virtualization() const
{
    return d->m_virtualization;
}

inline qulonglong CPUSet::l1iCache() const
{
    if (d->m_cpuInfoDB.size() > 0)
        return d->m_cpuInfoDB[0].l1iCache();

    return 0;
}

inline qulonglong CPUSet::l1dCache() const
{
    if (d->m_cpuInfoDB.size() > 0)
        return d->m_cpuInfoDB[0].l1dCache();

    return 0;
}

inline qulonglong CPUSet::l2Cache() const
{
    if (d->m_cpuInfoDB.size() > 0)
        return d->m_cpuInfoDB[0].l2Cache();

    return 0;
}

inline qulonglong CPUSet::l3Cache() const
{
    if (d->m_cpuInfoDB.size() > 0)
        return d->m_cpuInfoDB[0].l3Cache();

    return 0;
}

inline const CPUUsage CPUSet::usage() const
{
    return d->m_usage;
}

inline const CPUStat CPUSet::stat() const
{
    return d->m_stat;
}

inline QList<CPUInfo> CPUSet::cpuInfoDB() const
{
    return d->m_cpuInfoDB;
}

inline const CPUStat CPUSet::statDB(const QByteArray &cpu) const
{
    return d->m_statDB[cpu];
}

inline const CPUUsage CPUSet::usageDB(const QByteArray &cpu) const
{
    return d->m_usageDB[cpu];
}

} // namespace system
} // namespace core

#endif // CPUSET_H
