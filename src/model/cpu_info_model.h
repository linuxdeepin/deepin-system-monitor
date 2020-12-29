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
#include "system/cpu_set.h"
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
    explicit CPUInfoModel(const TimePeriod &period, QObject *parent = nullptr);
    virtual ~CPUInfoModel() override = default;

    std::weak_ptr<CPUListModel> cpuListModel() const;

    QString cpuFreq() const;
    QString maxFreq() const;
    QString minFreq() const;
    QString model() const;
    QString vendor() const;
    uint nCores() const;
    uint nSockets() const;
    uint nProcessors() const;
    QString virtualization() const;
    QString l1iCache() const;
    QString l1dCache() const;
    QString l2Cache() const;
    QString l3Cache() const;

    QString utilization() const;
    QString loadavg() const;
    uint nProcesses() const;
    uint nThreads() const;
    uint nFileDescriptors() const;
    QString hostname() const;
    QString osType() const;
    QString osVersion() const;
    QString uptime() const;

signals:
    void modelUpdated();

public slots:
    void updateModel();

private:
    TimePeriod m_period;
    std::unique_ptr<Sample<cpu_stat_t>> m_overallStatSample;
    std::unique_ptr<Sample<cpu_usage_t>> m_overallUsageSample;
    std::unique_ptr<Sample<load_avg_t>> m_loadAvgSampleDB; // for loadavg monitoring extends

    std::shared_ptr<CPUListModel> m_cpuListModel;

    SysInfo m_sysInfo;
    CPUSet m_cpuSet;
};

inline std::weak_ptr<CPUListModel> CPUInfoModel::cpuListModel() const
{
    return m_cpuListModel;
}

inline QString CPUInfoModel::cpuFreq() const
{
    return formatHz(m_cpuSet.curfreq());
}

inline QString CPUInfoModel::maxFreq() const
{
    return formatHz(m_cpuSet.maxfreq());
}

inline QString CPUInfoModel::minFreq() const
{
    return formatHz(m_cpuSet.minfreq());
}

inline QString CPUInfoModel::model() const
{
    return m_cpuSet.cpuModel();
}

inline QString CPUInfoModel::vendor() const
{
    return m_cpuSet.cpuVendor();
}

inline uint CPUInfoModel::nCores() const
{
    return m_cpuSet.cores();
}

inline uint CPUInfoModel::nSockets() const
{
    return m_cpuSet.sockets();
}

inline uint CPUInfoModel::nProcessors() const
{
    return m_cpuSet.processors();
}

inline QString CPUInfoModel::virtualization() const
{
    return m_cpuSet.virtualization();
}

inline QString CPUInfoModel::l1iCache() const
{
    return formatUnit({m_cpuSet.l1iCache()}, B, 0);
}

inline QString CPUInfoModel::l1dCache() const
{
    return formatUnit({m_cpuSet.l1dCache()}, B, 0);
}

inline QString CPUInfoModel::l2Cache() const
{
    return formatUnit({m_cpuSet.l2Cache()}, B, 0);
}

inline QString CPUInfoModel::l3Cache() const
{
    return formatUnit({m_cpuSet.l3Cache()}, B, 0);
}

inline QString CPUInfoModel::utilization() const
{
    auto usage = m_overallUsageSample->recentSample()->stat;
    return QString::number((usage->total - usage->idle) * 1. / usage->total * 100, 'f', 1).append('%');
}

inline QString CPUInfoModel::loadavg() const
{
    QString buffer {};
    buffer << *m_sysInfo.loadAvg();
    return buffer;
}

inline uint CPUInfoModel::nProcesses() const
{
    return m_sysInfo.nprocesses();
}

inline uint CPUInfoModel::nThreads() const
{
    return m_sysInfo.nthreads();
}

inline uint CPUInfoModel::nFileDescriptors() const
{
    return m_sysInfo.nfds();
}

inline QString CPUInfoModel::hostname() const
{
    return m_sysInfo.hostname();
}

inline QString CPUInfoModel::osType() const
{
    return m_sysInfo.arch();
}

inline QString CPUInfoModel::osVersion() const
{
    return m_sysInfo.version();
}

#endif // CPU_INFO_MODEL_H
