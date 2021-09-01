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
    QString coreID() const;
    QString modelName() const;
    QString vendorId() const;
    QString cpuFreq() const;
    QString cacheSize() const;

    QByteArray logicalName() const;

    // TODO: cache vary depends on different cpu arch, some dont have level3 cache
    QString l1iCache() const;
    QString l1dCache() const;
    QString l2Cache() const;
    QString l3Cache() const;

    void setIndex(int index);
    void setCoreId(int coreId);
    void setModelName(QString modelName);
    void setVendorId(QString vendorId);
    void setCpuFreq(QString cpuFreq);
    void setCacheSize(QString cacheSize);
private:
    QSharedDataPointer<CPUInfoPrivate> d;
};

} // namespace system
} // namespace core

Q_DECLARE_TYPEINFO(core::system::CPUInfo, Q_MOVABLE_TYPE);

#endif // CPU_H
