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
#include "cpu_info_model.h"

#include "cpu_stat_model.h"
#include "cpu_list_model.h"
#include "system/device_db.h"
#include "system/system_monitor.h"

CPUInfoModel::CPUInfoModel(const TimePeriod &period, QObject *parent)
    : QObject(parent)
    , m_period(period)
    , m_overallStatSample(new CPUStatSample(period))
    , m_overallUsageSample(new CPUUsageSample(period))
    , m_loadAvgSampleDB(new LoadAvgSample(period))
    , m_cpuListModel(new CPUListModel(this))
    , m_sysInfo {}
    , m_cpuSet {}
{
    auto *thread = ThreadManager::instance()->thread<SystemMonitorThread>(BaseThread::kSystemMonitorThread);
    if (thread) {
        auto *monitor = thread->threadJobInstance<SystemMonitor>();
        if (monitor) {
            //        connect(monitor.get(), &SystemMonitor::statInfoUpdated, this, &CPUInfoModel::updateModel);
        }
    }
}

QString CPUInfoModel::uptime() const
{
    QString buffer;
    time_t uptime = m_sysInfo.uptime().tv_sec;
    long days = uptime / 86400;
    long hours = (uptime - days * 86400) / 3600;
    long mins = (uptime - days * 86400 - hours * 3600) / 60;
    // TODO: move tr to i18n.cpp
    buffer = QString("%1, %2:%3")
                 .arg(QCoreApplication::translate("up %1 days(s)", "SysInfo.Uptime", nullptr, int(days)))
                 .arg(hours)
                 .arg(mins);
    return buffer;
}

void CPUInfoModel::updateModel()
{
    auto *thread = ThreadManager::instance()->thread<SystemMonitorThread>(BaseThread::kSystemMonitorThread);
    if (thread) {
        auto *monitor = thread->threadJobInstance<SystemMonitor>();
        if (monitor) {
            auto devDB = monitor->deviceDB().lock();
            if (devDB) {
                m_sysInfo = monitor->sysInfo();
                m_cpuSet = devDB->cpuSet();

                if (m_overallStatSample)
                    m_overallStatSample->addSample(new CPUStatSampleFrame(m_sysInfo.uptime(), m_cpuSet.stat()));

                if (m_overallUsageSample)
                    m_overallUsageSample->addSample(new CPUUsageSampleFrame(m_sysInfo.uptime(), m_cpuSet.usage()));

                if (m_loadAvgSampleDB)
                    m_loadAvgSampleDB->addSample(new LoadAvgSampleFrame(m_sysInfo.uptime(), m_sysInfo.loadAvg()));

                if (m_cpuListModel) {
                    m_cpuListModel->m_cpuInfoDB = m_cpuSet.cpuInfoDB();

                    for (auto &info : m_cpuListModel->m_cpuInfoDB) {
                        auto model = std::make_shared<CPUStatModel>(m_period, this);
                        if (model && model->m_statSampleDB)
                            model->m_statSampleDB->addSample(new CPUStatSampleFrame(m_sysInfo.uptime(), m_cpuSet.statDB(info.logicalName())));

                        if (model && model->m_usageSampleDB)
                            model->m_usageSampleDB->addSample(new CPUUsageSampleFrame(m_sysInfo.uptime(), m_cpuSet.usageDB(info.logicalName())));

                        m_cpuListModel->m_statModelDB[info.logicalName()] = model;
                    } // ::for
                } // ::m_cpuListModel

                emit modelUpdated();
            } // ::if(devDB)
        } // ::if(monitor)
    } // if(thread)
} // ::updateModel
