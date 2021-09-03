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

public://info
    QString modelName() const;

    QString vendor() const;

    int cpuCount() const;

    int socketCount() const;

    QString virtualization() const;

    QString curFreq() const;

    QString minFreq() const;

    QString maxFreq() const;

    QString l1dCache() const;

    QString l1iCache() const;

    QString l2Cache() const;

    QString l3Cache() const;

public://core
    QString coreId(int index) const;

public://usage
    const CPUUsage usage() const;

    const CPUStat stat() const;

    QList<QByteArray> cpuLogicName() const;

    const CPUStat statDB(const QByteArray &cpu) const;

    const CPUUsage usageDB(const QByteArray &cpu) const;

    qulonglong getUsageTotalDelta() const;

public:
    void update();

private:
    void read_stats();
    /**
     * @brief read_lscpu 通过lscpu读取CPU信息
     */
    void read_lscpu();
    void read_overall_info();

private:
    QSharedDataPointer<CPUSetPrivate> d;
};

} // namespace system
} // namespace core

#endif // CPUSET_H
