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
    , m_eventLoop(new UEventLoop(this))
    , m_desktopEntryCacheUpdater(new DesktopEntryCacheUpdater(this))
    , m_period(TimePeriod(TimePeriod::k1Min, timeval {2, 0})) /*TODO: change default setting from config*/
    , m_desktopEntryCacheUpdateInterval(timeval {15, 0}) /*TODO: change default setting from config*/
{
    scheduleUpdate(m_eventLoop, &m_period.interval());
    m_windowList->scheduleUpdate(m_eventLoop, &m_period.interval());
    // TODO: schedule netlink sock fd event

    // update desktop entry cache every 15 seconds
    m_desktopEntryCacheUpdater->scheduleUpdate(m_eventLoop, &m_desktopEntryCacheUpdateInterval);

    connect(this, &SystemMonitor::timePeriodChanged, [&](const TimePeriod &period) {
        // update event interval
        if (timercmp(&m_period.interval(), &period.interval(), !=)) {
            scheduleUpdate(m_eventLoop, &m_period.interval());
            m_windowList->scheduleUpdate(m_eventLoop, &m_period.interval());
        }
    });
}

SystemMonitor::~SystemMonitor()
{
    m_eventLoop->exit();
}

void SystemMonitor::startMonitorJob()
{
    m_eventLoop->run();
}

void SystemMonitor::requestInterrupt()
{
    m_eventLoop->exit();
}

void SystemMonitor::scheduleUpdate(UEventLoop *loop, const timeval *interval)
{
    loop->installTimerEventFilter(kInfoStatUpdateTimer, true, UEvent::kEventPriorityNormal, interval, this);
    loop->installTimerEventFilter(kProcessEventsTimer, true, UEvent::kEventPriorityLowest, interval, this);
}

// schedule update device & process info
bool SystemMonitor::uevent(UEvent *event)
{
    if (!event)
        return false;

    if (event->type == UEvent::kEventTypeTimer) {
        auto *ev = dynamic_cast<UTimerEvent *>(event);
        if (ev->timerId == kInfoStatUpdateTimer) {
            m_sysInfo->readSysInfo();
            m_deviceDB->update();
            m_processDB->update();

            emit statInfoUpdated();
            return true;
        } else if (ev->timerId == kProcessEventsTimer) {
            auto *evdsp = QAbstractEventDispatcher::instance(QThread::currentThread());
            if (evdsp) {
                evdsp->processEvents(QEventLoop::ExcludeUserInputEvents);
                return true;
            }
        }
    }

    return false;
}

void SystemMonitor::changeTimePeriod(const TimePeriod &period)
{
    setTimePeriod(period);
}

} // namespace system
} // namespace core
