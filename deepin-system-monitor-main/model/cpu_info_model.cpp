// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "cpu_info_model.h"

#include "system/cpu_set.h"
#include "cpu_list_model.h"
#include "system/device_db.h"
#include "system/system_monitor.h"
#include "system/sys_info.h"
#include "ddlog.h"

#include <QApplication>

using namespace DDLog;

Q_GLOBAL_STATIC(CPUInfoModel, theInstance)
CPUInfoModel::CPUInfoModel() : QObject(nullptr)
{
    qCDebug(app) << "CPUInfoModel constructor";
    m_period = TimePeriod(TimePeriod::kNoPeriod, {2, 0});
    m_overallStatSample.reset(new CPUStatSample(m_period));
    m_overallUsageSample.reset(new CPUUsageSample(m_period));
    m_loadAvgSampleDB.reset(new LoadAvgSample(m_period));

    m_sysInfo = SysInfo::instance();
    m_cpuSet = DeviceDB::instance()->cpuSet();

    connect(SystemMonitor::instance(), &SystemMonitor::statInfoUpdated, this, &CPUInfoModel::updateModel);
}

CPUInfoModel *CPUInfoModel::instance()
{
    qCDebug(app) << "CPUInfoModel::instance()";
    return theInstance();
}

QString CPUInfoModel::uptime() const
{
    qCDebug(app) << "CPUInfoModel::uptime()";
    QString buffer;
    time_t uptime = m_sysInfo->uptime().tv_sec;
    long days = uptime / 86400;
    long hours = (uptime - days * 86400) / 3600;
    long mins = (uptime - days * 86400 - hours * 3600) / 60;
    // TODO: move tr to i18n.cpp
    buffer = tr("%1 days %2 hours %3 minutes")
             .arg(days)
             .arg(hours)
             .arg(mins);
    return buffer;
}

SysInfo *CPUInfoModel::sysInfo()
{
    // qCDebug(app) << "CPUInfoModel::sysInfo()";
    return m_sysInfo;
}

CPUSet *CPUInfoModel::cpuSet()
{
    // qCDebug(app) << "CPUInfoModel::cpuSet()";
    return m_cpuSet;
}

void CPUInfoModel::updateModel()
{
    qCDebug(app) << "CPUInfoModel::updateModel()";
    m_overallStatSample->addSample(new CPUStatSampleFrame(m_sysInfo->uptime(), std::make_shared<struct cpu_stat_t>(*m_cpuSet->stat())));

    m_overallUsageSample->addSample(new CPUUsageSampleFrame(m_sysInfo->uptime(), std::make_shared<struct cpu_usage_t>(*m_cpuSet->usage())));

    m_loadAvgSampleDB->addSample(new LoadAvgSampleFrame(m_sysInfo->uptime(), std::make_shared<struct load_avg_t>(*m_sysInfo->loadAvg())));

    for (auto &cpuname : m_cpuSet->cpuLogicName()) {
        if (m_singleUsageSample.contains(cpuname)) {
            m_singleUsageSample[cpuname]->addSample(new CPUUsageSampleFrame(m_sysInfo->uptime(), std::make_shared<struct cpu_usage_t>(*m_cpuSet->usageDB(cpuname))));
        } else {
            // qCDebug(app) << "Creating new usage sample for cpu:" << cpuname;
            auto smaple = std::make_shared<Sample<cpu_usage_t>>(m_period);
            smaple->addSample(new CPUUsageSampleFrame(m_sysInfo->uptime(), std::make_shared<struct cpu_usage_t>(*m_cpuSet->usageDB(cpuname))));
            m_singleUsageSample.insert(cpuname, smaple);
        }

//        auto model = std::make_shared<CPUStatModel>(m_period, this);
//        if (model && model->m_statSampleDB)
//            model->m_statSampleDB->addSample(new CPUStatSampleFrame(m_sysInfo->uptime(), std::make_shared<struct cpu_stat_t>(*m_cpuSet->statDB(cpuname))));

//        if (model && model->m_usageSampleDB)
//            model->m_usageSampleDB->addSample(new CPUUsageSampleFrame(m_sysInfo->uptime(), std::make_shared<struct cpu_usage_t>(*m_cpuSet->usageDB(cpuname))));

//        m_cpuListModel->m_statModelDB[info.logicalName()] = model;
    } // ::for

    emit modelUpdated();
} // ::updateModel

QList<qreal> CPUInfoModel::cpuPercentList() const
{
    qCDebug(app) << "CPUInfoModel::cpuPercentList()";
    QList<qreal> percentList;
    const auto &usageSampleList = m_singleUsageSample.values();
    for (const auto &sample : usageSampleList) {
        const auto &pair = sample->recentSamplePair();
        percentList << CPUUsageSampleFrame::cpupc(pair.first, pair.second);
    }
    return percentList;
}

qreal CPUInfoModel::cpuAllPercent() const
{
    // qCDebug(app) << "CPUInfoModel::cpuAllPercent()";
    auto pair = m_overallUsageSample->recentSamplePair();
    return CPUUsageSampleFrame::cpupc(pair.first, pair.second);
}

QString CPUInfoModel::loadavg() const
{
    // qCDebug(app) << "CPUInfoModel::loadavg()";
    QString buffer {};
    buffer << *m_sysInfo->loadAvg();
    return buffer;
}

uint CPUInfoModel::nProcesses() const
{
    // qCDebug(app) << "CPUInfoModel::nProcesses()";
    return m_sysInfo->nprocesses();
}

uint CPUInfoModel::nThreads() const
{
    // qCDebug(app) << "CPUInfoModel::nThreads()";
    return m_sysInfo->nthreads();
}

uint CPUInfoModel::nFileDescriptors() const
{
    // qCDebug(app) << "CPUInfoModel::nFileDescriptors()";
    return m_sysInfo->nfds();
}

QString CPUInfoModel::hostname() const
{
    // qCDebug(app) << "CPUInfoModel::hostname()";
    return m_sysInfo->hostname();
}

QString CPUInfoModel::osType() const
{
    // qCDebug(app) << "CPUInfoModel::osType()";
    return m_sysInfo->arch();
}

QString CPUInfoModel::osVersion() const
{
    // qCDebug(app) << "CPUInfoModel::osVersion()";
    return m_sysInfo->version();
}
