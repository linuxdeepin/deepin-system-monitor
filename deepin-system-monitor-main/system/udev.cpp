// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "udev.h"
#include "ddlog.h"

#include <libudev.h>

using namespace DDLog;
namespace core {
namespace system {

UDev::UDev()
{
    qCDebug(app) << "Creating UDev object";
    m_udev = udev_new();
    if (!m_udev) {
        qCWarning(app) << "Failed to create udev context";
    }
}

UDev::~UDev()
{
    qCDebug(app) << "Destroying UDev object";
    if (m_udev) {
        udev_unref(m_udev);
    }
}

} // namespace system
} // namespace core
