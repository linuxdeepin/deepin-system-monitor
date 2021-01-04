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
#include "system_monitor.h"

#include "common/time_period.h"
#include "common/uevent_loop.h"
#include "device_db.h"
#include "process/process_db.h"
#include "process/desktop_entry_cache_updater.h"
#include "wm/wm_window_list.h"

#include <QAbstractEventDispatcher>
#include <QReadLocker>
#include <QWriteLocker>

using namespace common::core;

namespace core {
namespace system {

SystemMonitor::SystemMonitor(QObject *parent)
    : QObject(parent)
    , m_sysInfo(new SysInfo())
    , m_deviceDB(new DeviceDB())
    , m_processDB(new ProcessDB(this))
    , m_windowList(new WMWindowList(this))
    , m_desktopEntryCacheUpdater(new DesktopEntryCacheUpdater(this))
{

}

SystemMonitor::~SystemMonitor()
{
    m_basictimer.stop();
    delete m_sysInfo;
    delete m_deviceDB;
    delete m_processDB;
    delete m_windowList;
    delete m_desktopEntryCacheUpdater;
}

void SystemMonitor::startMonitorJob()
{
    m_basictimer.stop();
    m_basictimer.start(2000, Qt::VeryCoarseTimer, this);
}

void SystemMonitor::requestInterrupt()
{
    m_basictimer.stop();
}

void SystemMonitor::timerEvent(QTimerEvent *event)
{
    QObject::timerEvent(event);
    if (event->timerId() == m_basictimer.timerId()) {
        m_sysInfo->readSysInfo();
        m_deviceDB->update();
        m_processDB->update();

        emit statInfoUpdated();
    }
}

} // namespace system
} // namespace core
