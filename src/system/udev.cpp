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
#include "udev.h"

#include <libudev.h>

namespace core {
namespace system {

UDev::UDev()
{
    m_udev = udev_new();
}

UDev::~UDev()
{
    udev_unref(m_udev);
}

UDev::DeviceIterator UDev::enumDevicesWithSubSystem(const QByteArray &subsystem) const
{
    // TODO
    return {};
}

UDev::DeviceIterator UDev::enumDevicesWithSysAttr(const QByteArray &attr, const QByteArray &value) const
{
    // TODO
    return {};
}

UDev::DeviceIterator UDev::enumDevicesWithProperty(const QByteArray &prop, const QByteArray &value) const
{
    // TODO
    return {};
}

UDev::DeviceIterator UDev::enumDevicesWithSysName(const QByteArray &sysname) const
{
    // TODO
    return {};
}

UDev::DeviceIterator UDev::enumDevicesWithParent(const std::unique_ptr<UDevDevice> &parent) const
{
    // TODO
    return {};
}

} // namespace system
} // namespace core
