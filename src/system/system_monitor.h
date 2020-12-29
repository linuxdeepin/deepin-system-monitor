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
    , public UEventFilter
{
    Q_OBJECT

public:
    enum {
        kInfoStatUpdateTimer,
        kProcessEventsTimer
    };

    explicit SystemMonitor(QObject *parent = nullptr);
    virtual ~SystemMonitor();

    const TimePeriod &timePeriod() const;
    void setTimePeriod(const TimePeriod &period);
    void setDesktopEntryCacheUpdateInterval(const struct timeval &tv);

    std::weak_ptr<DeviceDB> deviceDB();
    std::weak_ptr<ProcessDB> processDB();
    std::weak_ptr<WMWindowList> windowList();
    SysInfo sysInfo();

    UEventLoop *eventLoop() const;
    void startMonitorJob();
    // atomic operation, can be called from main thread (gui) when exit requested
    void requestInterrupt();
    void scheduleUpdate(UEventLoop *loop, const struct timeval *interval);
    bool uevent(UEvent *event);

signals:
    void timePeriodChanged(const TimePeriod &period);
    void statInfoUpdated();

public slots:
    void changeTimePeriod(const TimePeriod &period);

private:
    std::weak_ptr<SysInfo> sysInfoRef();

    mutable QReadWriteLock m_rwlock;
    std::shared_ptr<SysInfo> m_sysInfo;
    std::shared_ptr<DeviceDB> m_deviceDB;
    std::shared_ptr<ProcessDB> m_processDB;
    std::shared_ptr<WMWindowList> m_windowList;

    UEventLoop *m_eventLoop;
    DesktopEntryCacheUpdater *m_desktopEntryCacheUpdater;

    TimePeriod m_period;
    struct timeval m_desktopEntryCacheUpdateInterval;

    friend class CPUSet;
};

inline const TimePeriod &SystemMonitor::timePeriod() const
{
    return m_period;
}

inline void SystemMonitor::setTimePeriod(const TimePeriod &period)
{
    if (m_period != period) {
        m_period = period;
        // queued connections are not supported, since libevent's eventloop will run forever before quit,
        // so slots in this thread will have no chance being called by Qt's eventloop by any means
        emit timePeriodChanged(m_period);
    }
}

inline void SystemMonitor::setDesktopEntryCacheUpdateInterval(const timeval &tv)
{
    m_desktopEntryCacheUpdateInterval = tv;
}

inline std::weak_ptr<DeviceDB> SystemMonitor::deviceDB()
{
    QReadLocker lock(&m_rwlock);
    return m_deviceDB;
}

inline std::weak_ptr<ProcessDB> SystemMonitor::processDB()
{
    QReadLocker lock(&m_rwlock);
    return m_processDB;
}

inline std::weak_ptr<WMWindowList> SystemMonitor::windowList()
{
    QReadLocker lock(&m_rwlock);
    return m_windowList;
}

inline SysInfo SystemMonitor::sysInfo()
{
    QReadLocker lock(&m_rwlock);
    return *m_sysInfo;
}

inline UEventLoop *SystemMonitor::eventLoop() const
{
    return m_eventLoop;
}

inline std::weak_ptr<SysInfo> SystemMonitor::sysInfoRef()
{
    return m_sysInfo;
}

} // namespace system
} // namespace core

#endif // SYSTEM_MONITOR_H
