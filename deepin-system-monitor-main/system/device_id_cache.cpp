// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "device_id_cache.h"
#include "ddlog.h"

#include "udev.h"

#include <libudev.h>
#include <cstring>

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

QByteArray DeviceIDCache::deviceName(const QByteArray &modalias) const
{
    if (modalias.isEmpty()) {
        return {};
    }

    auto it = m_cache.constFind(modalias);
    if (it != m_cache.constEnd()) {
        return it.value();
    }

    QByteArray name;
    if (m_hwdb) {
        struct udev_list_entry *entry = udev_hwdb_get_properties_list_entry(
            m_hwdb, modalias.constData(), 0);
        struct udev_list_entry *e;
        udev_list_entry_foreach(e, entry) {
            const char *key = udev_list_entry_get_name(e);
            if (key && strcmp(key, "ID_MODEL_FROM_DATABASE") == 0) {
                const char *val = udev_list_entry_get_value(e);
                if (val) {
                    name = QByteArray(val);
                    break;
                }
            }
        }
    }

    m_cache.insert(modalias, name);
    return name;
}

} // namespace system
} // namespace core
