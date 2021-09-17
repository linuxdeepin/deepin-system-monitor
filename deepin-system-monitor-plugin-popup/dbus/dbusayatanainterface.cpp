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
#include "dbusayatanainterface.h"
#include "gui/datadealsingleton.h"

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDebug>
#include <QFile>

// 以下这个问题可以避免单例的内存泄露问题
std::atomic<DBusAyatanaInterface *> DBusAyatanaInterface::s_Instance;
std::mutex DBusAyatanaInterface::m_mutex;

const QString SERVICE_NAME = "org.ayatana.bamf";
const QString SERVICE_PATH = "/org/ayatana/bamf/matcher";
const QString SERVICE_SUB_NAME = "org.ayatana.bamf.view";
const QString SERVICE_SUB_INTERFACE_NAME = "org.ayatana.bamf.matcher";
const QString DEEPIN_AI_ASSISTANT_PATH = "/usr/share/applications/desktop-ai-assistant.desktop";

DBusAyatanaInterface::DBusAyatanaInterface(QObject *parent)
    : QObject(parent)
{
    // 初始化dbus
    init();
}

void DBusAyatanaInterface::slotActiveApplicationChanged(QString path, QString app)
{
    //判断路径是否为空
    if (!path.isEmpty()) {
        QDBusInterface interface(SERVICE_NAME, path, SERVICE_SUB_NAME, QDBusConnection::sessionBus(), this);
        QString name;
        QDBusReply<QString> reply = interface.call("Name");
        QFile file(DEEPIN_AI_ASSISTANT_PATH);
        //判断路径文件是否存在
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return;
        }
        QByteArray fileByteArray = file.readAll();
        QString displayString(fileByteArray);
        file.close();
        // 判断文件是否包含指定字符串
        if (displayString.contains(reply.value())) {
            emit sigSendCloseWidget();
        }
    }
}

void DBusAyatanaInterface::init()
{
    // 1. 连接到dbus
    if (!QDBusConnection::sessionBus().isConnected()) {
        fprintf(stderr, "Cannot connect to the D-Bus session bus./n"
                "To start it, run:/n"
                "/teval `dbus-launch --auto-syntax`/n");
    }

    // 2. create interface
    QDBusConnection::sessionBus().connect(SERVICE_NAME, SERVICE_PATH, SERVICE_SUB_INTERFACE_NAME, "ViewOpened",
                                               this, SLOT(slotActiveApplicationChanged(QString, QString)));
}

