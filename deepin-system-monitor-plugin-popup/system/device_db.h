/*
* Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
*
* Author:      yukuan <yukuan@uniontech.com>
* Maintainer:  yukuan <yukuan@uniontech.com>
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

class MemInfo;
class CPUSet;
class NetInfo;
class DiskIOInfo;
class BlockDeviceInfoDB;

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
    DiskIOInfo *diskIoInfo();
    BlockDeviceInfoDB *blockDeviceInfoDB();
    NetInfo *netInfo();

    void update();

private:
    CPUSet *m_cpuSet;
    MemInfo *m_memInfo;
    NetInfo *m_netInfo;
    BlockDeviceInfoDB *m_blkDevInfoDB;
    DiskIOInfo *m_diskIoInfo;
};

} // namespace system
} // namespace core

#endif // DEVICE_DB_H
