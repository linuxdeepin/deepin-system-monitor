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
#include "mem_info_model.h"

#include "mem_stat_model.h"
#include "system/mem.h"
#include "system/system_monitor.h"
#include "system/device_db.h"
#include "system/sys_info.h"
#include "common/thread_manager.h"
#include "system/system_monitor.h"
#include "system/system_monitor_thread.h"

MemInfoModel::MemInfoModel(const TimePeriod &period, QObject *parent)
    : QObject(parent)
    , m_statModel(new MemStatModel(period, this))
{
    auto *monitor = ThreadManager::instance()->thread<SystemMonitorThread>(BaseThread::kSystemMonitorThread)->systemMonitorInstance();
    m_info = monitor->deviceDB()->memInfo();

    connect(monitor, &SystemMonitor::statInfoUpdated, this, &MemInfoModel::updateModel);
}

MemInfoModel::~MemInfoModel()
{
}

void MemInfoModel::updateModel()
{
    if (m_statModel->m_stat) {
        auto *monitor = ThreadManager::instance()->thread<SystemMonitorThread>(BaseThread::kSystemMonitorThread)->systemMonitorInstance();
        MemStat stat = std::make_shared<struct mem_stat_t>();
        stat->memUsed = m_info->memTotal() - m_info->memAvailable();
        stat->memTotal = m_info->memTotal();
        stat->swapUsed = m_info->swapTotal() - m_info->swapFree();
        stat->swapTotal = m_info->swapTotal();
        m_statModel->m_stat->addSample(new MemStatSampleFrame(monitor->sysInfo()->uptime(), stat));

        emit modelUpdated();
    }
} // ::updateModel
