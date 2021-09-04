/*
* Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd.
*
* Author:     yukuan  <yukuan@uniontech.com>
*
* Maintainer: yukuan  <yukuan@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "dbuscallmaininterface.h"

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDebug>

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
    if (QDBusConnection::sessionBus().isConnected()) {
        fprintf(stderr, "Cannot connect to the D-Bus session bus./n"
                "To start it, run:/n"
                "/teval `dbus-launch --auto-syntax`/n");
    }

    // 2. create interface
    mp_Iface = new QDBusInterface(SERVICE_NAME, SERVICE_PATH, "", QDBusConnection::sessionBus());
}
