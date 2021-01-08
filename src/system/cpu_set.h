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

#include <QList>
#include <QSharedDataPointer>

namespace core {
namespace system {

class CPUSetPrivate;
class CPUSet
{
    friend class Process;

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
    QList<QByteArray> cpuLogicName() const;
    // per cpu usage & stats
    const CPUStat statDB(const QByteArray &cpu) const;
    const CPUUsage usageDB(const QByteArray &cpu) const;

    qulonglong getUsageTotalDelta() const;

public:
    void update();

private:
    void read_stats();
    void read_overall_info();

private:
    QSharedDataPointer<CPUSetPrivate> d;
};

} // namespace system
} // namespace core

#endif // CPUSET_H
