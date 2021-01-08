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
#ifndef CPU_H
#define CPU_H

#include <QSharedDataPointer>

#include <memory>
#include <QDebug>

namespace core {
namespace system {

// from /proc/stat, ref: sysstat#rd_stats.h#stats_cpu
struct cpu_stat_t {
    QByteArray cpu {};
    unsigned long long user {0}; // user time
    unsigned long long nice {0}; // user time with low priority
    unsigned long long sys {0}; // system time
    unsigned long long idle {0}; // idle time
    unsigned long long iowait {0}; // io wait time
    unsigned long long hardirq {0}; // interrupt time
    unsigned long long softirq {0}; // soft interrupt time
    unsigned long long steal {0}; // stolen time
    unsigned long long guest {0}; // guest time
    unsigned long long guest_nice {0}; // guest time (niced)
};

struct cpu_usage_t {
    QByteArray cpu {};
    unsigned long long total {0};
    unsigned long long idle {0};
};

using CPUStat = std::shared_ptr<struct cpu_stat_t>;
using CPUUsage = std::shared_ptr<struct cpu_usage_t>;

class CPUSet;
class CPUInfoPrivate;

class CPUInfo
{
public:
    explicit CPUInfo();
    explicit CPUInfo(int index);
    CPUInfo(const CPUInfo &other);
    CPUInfo &operator=(const CPUInfo &rhs);
    ~CPUInfo();

    int logicalIndex() const;
    QByteArray logicalName() const;

    quint32 curfreq() const;
    quint32 minfreq() const;
    quint32 maxfreq() const;

    int packageID() const;
    int coreID() const;
    int bookID() const;
    int drawerID() const;

    // TODO: cache vary depends on different cpu arch, some dont have level3 cache
    qulonglong l1iCache() const;
    qulonglong l1dCache() const;
    qulonglong l2Cache() const;
    qulonglong l3Cache() const;

protected:
    void readCPUInfo(int index);

private:
    void read_min_freq(int index);
    void read_max_freq(int index);
    void read_cur_freq(int index);
    void read_cache(int index);
    void read_topology(int index);

private:
    QSharedDataPointer<CPUInfoPrivate> d;
};

} // namespace system
} // namespace core

Q_DECLARE_TYPEINFO(core::system::CPUInfo, Q_MOVABLE_TYPE);

#endif // CPU_H
