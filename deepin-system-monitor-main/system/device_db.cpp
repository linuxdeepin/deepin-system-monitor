// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "device_db.h"
#include "ddlog.h"

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

using namespace DDLog;

namespace core {
namespace system {

DeviceDB::DeviceDB()
{
    qCDebug(app) << "DeviceDB constructor: Initializing all device info objects...";
    m_cpuSet = new CPUSet();
    m_memInfo = new MemInfo();
    m_netifInfoDB = new NetifInfoDB();
    m_blkDevInfoDB = new BlockDeviceInfoDB();
    m_diskIoInfo = new DiskIOInfo();
    m_netInfo = new NetInfo();
    qCDebug(app) << "DeviceDB construction finished.";
}

DeviceDB::~DeviceDB()
{
    // qCDebug(app) << "DeviceDB destructor: Cleaning up all device info objects...";
    if (m_diskIoInfo) {
        // qCDebug(app) << "DeviceDB destructor: Deleting DiskIOInfo.";
        delete m_diskIoInfo;
        m_diskIoInfo  = nullptr;
    }
    if (m_memInfo) {
        // qCDebug(app) << "DeviceDB destructor: Deleting MemInfo.";
        delete m_memInfo;
        m_memInfo  = nullptr;
    }
    if (m_cpuSet) {
        // qCDebug(app) << "DeviceDB destructor: Deleting CPUSet.";
        delete m_cpuSet;
        m_cpuSet  = nullptr;
    }
    if (m_blkDevInfoDB) {
        // qCDebug(app) << "DeviceDB destructor: Deleting BlockDeviceInfoDB.";
        delete m_blkDevInfoDB;
        m_blkDevInfoDB  = nullptr;
    }
    if (m_netifInfoDB) {
        // qCDebug(app) << "DeviceDB destructor: Deleting NetifInfoDB.";
        delete m_netifInfoDB;
        m_netifInfoDB  = nullptr;
    }
    if (m_netInfo) {
        // qCDebug(app) << "DeviceDB destructor: Deleting NetInfo.";
        delete m_netInfo;
        m_netInfo  = nullptr;
    }
    // qCDebug(app) << "DeviceDB destruction finished.";
}

void DeviceDB::update()
{
    qCDebug(app) << "DeviceDB update: Triggering update on all device info objects...";
    m_cpuSet->update();
    m_memInfo->readMemInfo();
    m_netifInfoDB->update();
    m_blkDevInfoDB->update();
    m_diskIoInfo->update();
    m_netInfo->resdNetInfo();
    qCDebug(app) << "DeviceDB update finished.";
}

DeviceDB *DeviceDB::instance()
{
    // qCDebug(app) << "DeviceDB instance: Getting instance...";
    auto *monitor = ThreadManager::instance()->thread<SystemMonitorThread>(BaseThread::kSystemMonitorThread)->systemMonitorInstance();
    return monitor->deviceDB();
}

BlockDeviceInfoDB *DeviceDB::blockDeviceInfoDB()
{
    // qCDebug(app) << "DeviceDB: Getting BlockDeviceInfoDB.";
    return m_blkDevInfoDB;
}

NetifInfoDB *DeviceDB::netifInfoDB()
{
    // qCDebug(app) << "DeviceDB: Getting NetifInfoDB.";
    return m_netifInfoDB;
}

MemInfo *DeviceDB::memInfo()
{
    // qCDebug(app) << "DeviceDB: Getting MemInfo.";
    return m_memInfo;
}

CPUSet *DeviceDB::cpuSet()
{
    // qCDebug(app) << "DeviceDB: Getting CPUSet.";
    return m_cpuSet;
}

DiskIOInfo *DeviceDB::diskIoInfo()
{
    // qCDebug(app) << "DeviceDB: Getting DiskIOInfo.";
    return m_diskIoInfo;
}

NetInfo *DeviceDB::netInfo()
{
    // qCDebug(app) << "DeviceDB: Getting NetInfo.";
    return m_netInfo;
}

} // namespace system
} // namespace core
