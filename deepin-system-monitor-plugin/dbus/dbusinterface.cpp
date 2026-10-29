// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd.
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dbusinterface.h"
#include "ddlog.h"

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDebug>
#include <QProcess>

using namespace DDLog;

// 以下这个问题可以避免单例的内存泄露问题
std::atomic<DBusInterface *> DBusInterface::s_Instance;
std::mutex DBusInterface::m_mutex;

const QString SERVICE_NAME = "com.deepin.SystemMonitorPluginPopup";
const QString SERVICE_PATH = "/com/deepin/SystemMonitorPluginPopup";

const QString DBUS_COMMAND = "gdbus call -e -d com.deepin.SystemMonitorPluginPopup -o /com/deepin/SystemMonitorPluginPopup -m  com.deepin.SystemMonitorPluginPopup.slotShowOrHideSystemMonitorPluginPopupWidget";

DBusInterface::DBusInterface()
    : mp_Iface(nullptr)
{
    // 初始化dbus
//    init();
}

void DBusInterface::showOrHideDeepinSystemMonitorPluginPopupWidget()
{
    // 调用dbus接口弹出插件主界面
    // 如果无效的话就执行一次唤醒DBUS服务的命令
    if (mp_Iface == nullptr || !mp_Iface->isValid()) {
        if (!QProcess::startDetached(DBUS_COMMAND)) {
            if (mp_Iface == nullptr) {
                init();
                if (!mp_Iface->isValid()) {
                    QDBusReply<void> reply = mp_Iface->call("slotShowOrHideSystemMonitorPluginPopupWidget");
                }
            }
        }
    } else {
        QDBusReply<void> reply = mp_Iface->call("slotShowOrHideSystemMonitorPluginPopupWidget");
    }
}

void DBusInterface::init()
{
    // 1. 连接到dbus
    if (!QDBusConnection::sessionBus().isConnected()) {
        qCWarning(app) << "Cannot connect to the D-Bus session bus. To start it, run: eval `dbus-launch --auto-syntax`";
    }

    // 2. create interface
    mp_Iface = new QDBusInterface(SERVICE_NAME, SERVICE_PATH, "", QDBusConnection::sessionBus());
}
