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
#ifndef BLOCK_DEVICE_INFO_DB_H
#define BLOCK_DEVICE_INFO_DB_H

#include "block_device.h"

#include <QReadWriteLock>
#include <QList>

namespace core {
namespace system {


class DeviceDB;

/**
 * @brief The BlockDeviceInfoDB class
 */
class BlockDeviceInfoDB
{
public:
    explicit BlockDeviceInfoDB();
    ~BlockDeviceInfoDB();

    QList<BlockDevice> deviceList();

    void update();

private:
    void readDiskInfo();

private:
    mutable QReadWriteLock m_rwlock;
    QList<BlockDevice> m_deviceList;
};

inline QList<BlockDevice> BlockDeviceInfoDB::deviceList()
{
    QReadLocker lock(&m_rwlock);
    return m_deviceList;
}

} // namespace system
} // namespace core

#endif // BLOCK_DEVICE_INFO_DB_H
