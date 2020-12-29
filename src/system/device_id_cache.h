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
#ifndef DEVICE_ID_CACHE_H
#define DEVICE_ID_CACHE_H

#include <QByteArray>
#include <QHash>

struct udev_hwdb;

namespace core {
namespace system {

class UDev;

class DeviceIDCache
{
public:
    explicit DeviceIDCache(const UDev *udev);
    virtual ~DeviceIDCache();

    QByteArray deviceName(const QByteArray &modalias) const;

private:
    struct udev_hwdb *m_hwdb;
    QHash<QByteArray, QByteArray> m_cache; // modalias : product_name
};

} // namespace system
} // namespace core

#endif // DEVICE_ID_CACHE_H
