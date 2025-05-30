// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd.
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dbuscallmaininterface.h"
#include "ddlog.h"

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDebug>

using namespace DDLog;

// 以下这个问题可以避免单例的内存泄露问题
std::atomic<DbusCallMainInterface *> DbusCallMainInterface::s_Instance;
std::mutex DbusCallMainInterface::m_mutex;

const QString SERVICE_NAME = "com.deepin.SystemMonitorMain";
const QString SERVICE_PATH = "/com/deepin/SystemMonitorMain";

DbusCallMainInterface::DbusCallMainInterface()
    : mp_Iface(nullptr)
{
    // 初始化dbus
    init();
}

void DbusCallMainInterface::jumpWidget(QString msgCode)
{
    // 调用dbus接口弹出插件主界面
    if (msgCode != "MSG_PROCESS")
        QDBusReply<void> reply = mp_Iface->call("slotJumpDetailWidget", msgCode);
    else
        QDBusReply<void> reply = mp_Iface->call("slotJumpProcessWidget", msgCode);
}

void DbusCallMainInterface::init()
{
    // 1. 连接到dbus
    if (!QDBusConnection::sessionBus().isConnected()) {
        qCWarning(app) << "Cannot connect to the D-Bus session bus. To start it, run: eval `dbus-launch --auto-syntax`";
    }

    qCDebug(app) << "Creating DBus interface for service:" << SERVICE_NAME << "path:" << SERVICE_PATH;
    // 2. create interface
    mp_Iface = new QDBusInterface(SERVICE_NAME, SERVICE_PATH, "", QDBusConnection::sessionBus());
}
