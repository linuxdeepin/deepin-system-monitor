/*
* Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd.
*
* Author:     yukuan<yukuan@uniontech.com>
* Maintainer: yukuan<yukuan@uniontech.com>
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
#include "dbus_object.h"
#include "gui/main_window.h"
#include "application.h"

#include <QDBusConnection>
#include <QDebug>

#define DBUS_SERVER             "com.deepin.systemMonitor"
#define DBUS_SERVER_PATH        "/com/deepin/systemMonitor"

QMutex DBusObject::mutex;
QAtomicPointer<DBusObject> DBusObject::instance;

DBusObject &DBusObject::getInstance()
{
    if (instance.testAndSetOrdered(nullptr, nullptr))
    {
        QMutexLocker locker(&mutex);

        instance.testAndSetOrdered(nullptr, new DBusObject);
    }
    return *instance;
}

bool DBusObject::registerOrNotify()
{
    QDBusConnection dbus = QDBusConnection::sessionBus();
    if (!dbus.registerService(DBUS_SERVER)) {
        QDBusInterface notification(DBUS_SERVER, DBUS_SERVER_PATH, DBUS_SERVER, QDBusConnection::sessionBus());
        QList<QVariant> args;
        QString error = notification.callWithArgumentList(QDBus::Block, "handleWindow", args).errorMessage();
        if (!error.isEmpty())
            qInfo() << error;
        return false;
    }

    dbus.registerObject(DBUS_SERVER_PATH, this, QDBusConnection::ExportScriptableSlots);

    return true;
}

void DBusObject::unRegister()
{
    internalMutex.lockForRead();
    QDBusConnection dbus = QDBusConnection::sessionBus();
    dbus.unregisterService(DBUS_SERVER);
    internalMutex.unlock();
}

void DBusObject::handleWindow()
{
    internalMutex.lockForRead();
    MainWindow *mw = gApp->mainWindow();
    mw->setWindowState((mw->windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
    internalMutex.unlock();
}

DBusObject::DBusObject(QObject *parent) : QObject(parent)
{

}

DBusObject::~DBusObject()
{

}
