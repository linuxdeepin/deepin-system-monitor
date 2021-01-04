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
#ifndef SYSTEM_MONITOR_THREAD_H
#define SYSTEM_MONITOR_THREAD_H

#include "common/base_thread.h"
#include "system/system_monitor.h"

#include <QReadWriteLock>

#include <memory>

using namespace common::core;

namespace core {
namespace system {

class SystemMonitor;

class SystemMonitorThread : public BaseThread
{
    Q_OBJECT

public:
    explicit SystemMonitorThread(QObject *parent = nullptr);
    ~SystemMonitorThread();

    int threadKey() const final;

public:
    SystemMonitor *systemMonitorInstance() const;

private:
    SystemMonitor *m_monitor;
};

inline int SystemMonitorThread::threadKey() const
{
    return kSystemMonitorThread;
}

} // namespace system
} // namespace core

#endif // SYSTEM_MONITOR_THREAD_H
