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
#ifndef CPUSET_H
#define CPUSET_H

#include <QList>
#include <QSharedDataPointer>

#include <memory>
#include <QDebug>

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

class CPUSet
{
    friend class Process;
public:
    CPUSet();
    ~CPUSet();

    CPUStat m_stat; // overall stat
    CPUUsage m_usage; // overall usage

//    QMap<QByteArray, cpu_stat_t> m_statDB; // per cpu stat
//    QMap<QByteArray, cpu_usage_t> m_usageDB; // per cpu usage

//    QList<CPUInfo> m_infos;         //per cpu info

private:
    void read_stats();


};

#endif // CPU_SET_P_H
