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
#ifndef UDEV_H
#define UDEV_H

#include <QByteArray>

#include <memory>

struct udev;

namespace core {
namespace system {

class UDevDevice;

class UDev
{
public:
    using HANDLE = struct udev *;

    explicit UDev();
    virtual ~UDev();

    // subsystem phy80211 &

    // ifname => vendor & product name

    //subsystem: net => enumerate
    //udev_device_new_from_subsystem_sysname
    //parent device => subsystem (pci / usb)?
    // pci: => vendor & device
    // => udev_database
    // usb: => DEVTYPE=usb_interface => parent device
    // => idProduct & idVendor & subsystem(usb) & DEVTYPE=usb_device
    // => udev_database
    // ===> cache

    // disk => model name & size & sector size

    HANDLE handle() const;

private:
    HANDLE m_udev;
};

inline UDev::HANDLE UDev::handle() const
{
    return m_udev;
}

} // namespace system
} // namespace core

#endif // UDEV_H
