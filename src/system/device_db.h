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

namespace core {
namespace system {

class BlockDeviceInfoDB;
class NetifInfoDB;
class MemInfo;
class CPUSet;
class SystemMonitor;
class DiskIOInfo;
class NetInfo;

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
    DiskIOInfo *diskIoInfo();
    NetInfo *netInfo();

    void update();

private:
    CPUSet *m_cpuSet;
    MemInfo *m_memInfo;
    NetifInfoDB *m_netifInfoDB;
    BlockDeviceInfoDB *m_blkDevInfoDB;
    DiskIOInfo *m_diskIoInfo;
    NetInfo *m_netInfo;
};

} // namespace system
} // namespace core

#endif // DEVICE_DB_H
