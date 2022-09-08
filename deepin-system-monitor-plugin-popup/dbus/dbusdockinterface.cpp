// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dbusdockinterface.h"

/*
 * Implementation of interface class DBusDockInterface
 */

DBusDockInterface::DBusDockInterface(QObject *parent)
    : QDBusAbstractInterface("com.deepin.dde.Dock", "/com/deepin/dde/Dock", staticInterfaceName(), QDBusConnection::sessionBus(), parent)
{
    QDBusConnection::sessionBus().connect(this->service(), this->path(), "org.freedesktop.DBus.Properties",  "PropertiesChanged","sa{sv}as", this, SLOT(__propertyChanged__(QDBusMessage)));
}

DBusDockInterface::~DBusDockInterface()
{
}

