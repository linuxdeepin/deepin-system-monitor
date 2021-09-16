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

#include <QObject>
#include <QBasicTimer>

namespace core {
namespace process {
class ProcessDB;
} // namespace process
} // namespace core

using namespace core::process;

namespace core {
namespace system {

class DeviceDB;
class SysInfo;

class SystemMonitor : public QObject
{
    Q_OBJECT
    friend class CPUSet;

signals:
    void statInfoUpdated();

public:
    explicit SystemMonitor(QObject *parent = nullptr);
    virtual ~SystemMonitor();

    static SystemMonitor *instance();

    SysInfo *sysInfo();
    DeviceDB *deviceDB();
    ProcessDB *processDB();

    void startMonitorJob();

protected:
    void timerEvent(QTimerEvent *event);

private:
    void updateSystemMonitorInfo();

private:
    SysInfo      *m_sysInfo;
    DeviceDB     *m_deviceDB;
    ProcessDB    *m_processDB;

    QBasicTimer m_basictimer;
};

} // namespace system
} // namespace core

#endif // SYSTEM_MONITOR_H
