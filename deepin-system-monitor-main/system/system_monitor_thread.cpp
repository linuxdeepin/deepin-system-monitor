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
#include "system_monitor_thread.h"

#include "system/system_monitor.h"
#include "process/process_db.h"

#include <QReadLocker>
#include <QWriteLocker>
#include <QDebug>
#include <QTimer>

namespace core {
namespace system {

SystemMonitorThread::SystemMonitorThread(QObject *parent)
    : BaseThread(parent)
    , m_monitor(new SystemMonitor())
{
    m_monitor->moveToThread(this);
    connect(this, &QThread::finished, this, &QObject::deleteLater);
    connect(this, &QThread::started, m_monitor, &SystemMonitor::startMonitorJob);
}

SystemMonitorThread::~SystemMonitorThread()
{
    m_monitor->deleteLater();
    quit();
    wait();
}

SystemMonitor *SystemMonitorThread::systemMonitorInstance() const
{
    return m_monitor;
}

} // namespace system
} // namespace core
