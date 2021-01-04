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
}

DeviceDB::~DeviceDB()
{
    delete m_memInfo;
    delete m_cpuSet;
    delete m_blkDevInfoDB;
    delete m_netifInfoDB;
}

void DeviceDB::update()
{
    QWriteLocker lock(&m_rwlock);
    m_cpuSet->update();
    m_memInfo->readMemInfo();
    m_netifInfoDB->update();
    m_blkDevInfoDB->update();
}

} // namespace system
} // namespace core
