// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "device_id_cache.h"
#include "ddlog.h"

#include "udev.h"

#include <libudev.h>

using namespace DDLog;

namespace core {
namespace system {

DeviceIDCache::DeviceIDCache(const UDev *udev)
{
    qCDebug(app) << "Creating DeviceIDCache";
    m_hwdb = udev_hwdb_new(udev->handle());
}

DeviceIDCache::~DeviceIDCache()
{
    qCDebug(app) << "Destroying DeviceIDCache";
    udev_hwdb_unref(m_hwdb);
}

} // namespace system
} // namespace core
