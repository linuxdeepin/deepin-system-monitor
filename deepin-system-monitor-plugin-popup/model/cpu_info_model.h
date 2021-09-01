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
#ifndef CPU_INFO_MODEL_H
#define CPU_INFO_MODEL_H

#include "common/sample.h"
#include "common/common.h"
#include "system/sys_info.h"
#include "cpu_stat_model.h"

#include <QObject>
#include <QMap>

#include <memory>

using namespace common::core;
using namespace common::format;
using namespace core::system;

template<>
class SampleFrame<load_avg_t>
{
public:
    SampleFrame()
        : ts()
        , stat()
    {
    }
    SampleFrame(const struct timeval &tv, const LoadAvg &st)
        : ts(tv)
        , stat(st)
    {
    }
    SampleFrame(const SampleFrame &other)
        : ts(other.ts)
        , stat(std::make_shared<load_avg_t>(*(other.stat)))
    {
    }

    struct timeval ts;
    LoadAvg stat;
};
using LoadAvgSampleFrame = SampleFrame<load_avg_t>;
using LoadAvgSample = Sample<load_avg_t>;

class CPUListModel;

class CPUInfoModel : public QObject
{
    Q_OBJECT

public:
    explicit CPUInfoModel();
    virtual ~CPUInfoModel() override = default;

    static CPUInfoModel *instance();

    std::weak_ptr<CPUListModel> cpuListModel() const;

    QList<qreal> cpuPercentList() const;
    qreal cpuAllPercent() const;

    QString loadavg() const;
    uint nProcesses() const;
    uint nThreads() const;
    uint nFileDescriptors() const;
    QString hostname() const;
    QString osType() const;
    QString osVersion() const;
    QString uptime() const;

    SysInfo *sysInfo();
    CPUSet *cpuSet();

signals:
    void modelUpdated();

public slots:
    void updateModel();

private:
    TimePeriod m_period;
    std::unique_ptr<Sample<cpu_stat_t>> m_overallStatSample;
    std::unique_ptr<Sample<cpu_usage_t>> m_overallUsageSample;
    std::unique_ptr<Sample<load_avg_t>> m_loadAvgSampleDB; // for loadavg monitoring extends

    QMap<QByteArray, std::shared_ptr<Sample<cpu_usage_t>>> m_singleUsageSample;

    SysInfo *m_sysInfo;
    CPUSet *m_cpuSet;
};

#endif // CPU_INFO_MODEL_H
