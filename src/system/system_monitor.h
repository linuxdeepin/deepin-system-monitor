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
#ifndef SYSTEM_MONITOR_H
#define SYSTEM_MONITOR_H

#include "common/uevent_filter.h"
#include "common/time_period.h"
#include "sys_info.h"

#include <QObject>
#include <QReadWriteLock>
#include <QBasicTimer>

#include <memory>

#include <sys/time.h>

namespace core {
namespace process {
class DesktopEntryCacheUpdater;
class ProcessDB;
} // namespace process
} // namespace core

namespace common {
namespace core {
class UEventLoop;
}
} // namespace common

namespace core {
namespace wm {
class WMWindowList;
}
} // namespace core

using namespace common::core;
using namespace core::process;
using namespace core::wm;
using namespace core::system;

namespace core {
namespace system {

class DeviceDB;
class SysInfo;

class SystemMonitor : public QObject
{
    Q_OBJECT

signals:
    void statInfoUpdated();

public:
    enum {
        kInfoStatUpdateTimer,
        kProcessEventsTimer
    };

    explicit SystemMonitor(QObject *parent = nullptr);
    virtual ~SystemMonitor();

    SysInfo *sysInfo();
    DeviceDB *deviceDB();
    ProcessDB *processDB();
    WMWindowList *windowList();

    void startMonitorJob();
    // atomic operation, can be called from main thread (gui) when exit requested
    void requestInterrupt();
    void scheduleUpdate(UEventLoop *loop, const struct timeval *interval);

protected:
    void timerEvent(QTimerEvent *event);

private:
    SysInfo      *m_sysInfo;
    DeviceDB     *m_deviceDB;
    ProcessDB    *m_processDB;
    WMWindowList *m_windowList;

    DesktopEntryCacheUpdater *m_desktopEntryCacheUpdater;

    friend class CPUSet;

    QBasicTimer m_basictimer;
};

inline DeviceDB *SystemMonitor::deviceDB()
{
    return m_deviceDB;
}

inline ProcessDB *SystemMonitor::processDB()
{
    return m_processDB;
}

inline WMWindowList *SystemMonitor::windowList()
{
    return m_windowList;
}

inline SysInfo *SystemMonitor::sysInfo()
{
    return m_sysInfo;
}

} // namespace system
} // namespace core

#endif // SYSTEM_MONITOR_H
