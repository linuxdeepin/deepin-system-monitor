// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "udev_device.h"
#include "udev.h"
#include "ddlog.h"

#include <libudev.h>

using namespace DDLog;
namespace core {
namespace system {

UDevDevice::UDevDevice(const UDev *udev, const char *path)
{
    qCDebug(app) << "Creating UDevDevice from syspath:" << path;
    m_device = udev_device_new_from_syspath(udev->handle(), path);
    if (!m_device) {
        qCWarning(app) << "Failed to create udev_device from path:" << path;
    }
}

UDevDevice::UDevDevice(HANDLE udevice)
{
    qCDebug(app) << "Creating UDevDevice from existing handle";
    m_device = udevice;
}

UDevDevice::~UDevDevice()
{
    qCDebug(app) << "Destroying UDevDevice";
    if (m_device) {
        udev_device_unref(m_device);
    }
}

} // namespace system
} // namespace core
