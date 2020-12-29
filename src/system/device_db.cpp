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
    : m_blkDevInfoDB(new BlockDeviceInfoDB())
    , m_netifInfoDB(new NetifInfoDB())
    , m_memInfo {}
    , m_cpuSet(/*TODO*/)
{
}

DeviceDB::~DeviceDB()
{
}

// TODO:
void DeviceDB::update()
{
    MemInfo mem;
    mem.readMemInfo();

    auto blkDevDB = std::make_shared<BlockDeviceInfoDB>();
    if (blkDevDB)
        blkDevDB->update();

    auto netifDB = std::make_shared<NetifInfoDB>();
    if (netifDB)
        netifDB->update();

    QWriteLocker lock(&m_rwlock);
    // update info
    m_memInfo = mem;
    m_blkDevInfoDB = blkDevDB;
    m_netifInfoDB = netifDB;
}

} // namespace system
} // namespace core
