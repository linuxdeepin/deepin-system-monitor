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
#include "device_db.h"

#include "cpu_set.h"
#include "mem.h"
#include "block_device_info_db.h"
#include "netif_info_db.h"
#include "diskio_info.h"
#include "net_info.h"
#include "common/thread_manager.h"
#include "system/system_monitor.h"
#include "system/system_monitor_thread.h"

#include <QSharedData>
#include <QReadLocker>
#include <QWriteLocker>

namespace core {
namespace system {

DeviceDB::DeviceDB()
{
    m_cpuSet = new CPUSet();
    m_memInfo = new MemInfo();
    m_netifInfoDB = new NetifInfoDB();
    m_blkDevInfoDB = new BlockDeviceInfoDB();
    m_diskIoInfo = new DiskIOInfo();
    m_netInfo = new NetInfo();
}

DeviceDB::~DeviceDB()
{
    delete m_diskIoInfo;
    delete m_memInfo;
    delete m_cpuSet;
    delete m_blkDevInfoDB;
    delete m_netifInfoDB;
    delete m_netInfo;
}

void DeviceDB::update()
{
    m_cpuSet->update();
    m_memInfo->readMemInfo();
    m_netifInfoDB->update();
    m_blkDevInfoDB->update();
    m_diskIoInfo->update();
    m_netInfo->resdNetInfo();
}

DeviceDB *DeviceDB::instance()
{
    auto *monitor = ThreadManager::instance()->thread<SystemMonitorThread>(BaseThread::kSystemMonitorThread)->systemMonitorInstance();
    return monitor->deviceDB();
}

BlockDeviceInfoDB *DeviceDB::blockDeviceInfoDB()
{
    return m_blkDevInfoDB;
}

NetifInfoDB *DeviceDB::netifInfoDB()
{
    return m_netifInfoDB;
}

MemInfo *DeviceDB::memInfo()
{
    return m_memInfo;
}

CPUSet *DeviceDB::cpuSet()
{
    return m_cpuSet;
}

DiskIOInfo *DeviceDB::diskIoInfo()
{
    return m_diskIoInfo;
}

NetInfo *DeviceDB::netInfo()
{
    return m_netInfo;
}

} // namespace system
} // namespace core
