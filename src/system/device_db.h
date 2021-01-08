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
#ifndef DEVICE_DB_H
#define DEVICE_DB_H

#include "block_device_info_db.h"
#include "netif_info_db.h"
#include "mem.h"
#include "cpu_set.h"
#include "common/thread_manager.h"
#include "system/system_monitor.h"
#include "system/system_monitor_thread.h"

#include <QReadWriteLock>

namespace core {
namespace system {

class BlockDeviceInfoDB;
class NetifInfoDB;
class MemInfo;
class CPUSet;
class SystemMonitor;

/**
 * @brief The DeviceDB class
 */
class DeviceDB
{
public:
    explicit DeviceDB();
    ~DeviceDB();

    static DeviceDB *instance();

    CPUSet *cpuSet();
    MemInfo *memInfo();
    NetifInfoDB *netifInfoDB();
    BlockDeviceInfoDB *blockDeviceInfoDB();

    void update();

private:
    // TODO: netlink

    mutable QReadWriteLock m_rwlock;

    CPUSet *m_cpuSet;
    MemInfo *m_memInfo;
    NetifInfoDB *m_netifInfoDB;
    BlockDeviceInfoDB *m_blkDevInfoDB;
};

inline DeviceDB *DeviceDB::instance()
{
    auto *monitor = ThreadManager::instance()->thread<SystemMonitorThread>(BaseThread::kSystemMonitorThread)->systemMonitorInstance();
    return monitor->deviceDB();
}

inline BlockDeviceInfoDB *DeviceDB::blockDeviceInfoDB()
{
    return m_blkDevInfoDB;
}

inline NetifInfoDB *DeviceDB::netifInfoDB()
{
    return m_netifInfoDB;
}

inline MemInfo *DeviceDB::memInfo()
{
    return m_memInfo;
}

inline CPUSet *DeviceDB::cpuSet()
{
    return m_cpuSet;
}

} // namespace system
} // namespace core

#endif // DEVICE_DB_H
