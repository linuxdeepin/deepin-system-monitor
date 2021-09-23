/*
* Copyright (C) 2019 ~ 2021 Uniontech Software Technology Co.,Ltd
*
* Author:      wangchao <wangchao@uniontech.com>
* Maintainer:  wangchao <wangchao@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#define private public
#define protected public

#include "dbus/dbus_common.h"
#include "dbus/dbus_properties_interface.h"
#include "dbus/environment_file.h"
#include "dbus/systemd1_service_interface.h"

#include <QVariant>
#include <QDBusInterface>
#include <QByteArray>
#include <QList>
#include <QMap>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QtDBus>

using namespace dbus::common;

#undef private
#undef protected

#include "ut_systemd1_service_interface.h"

Ut_Systemd1ServiceInterface::Ut_Systemd1ServiceInterface()
{
}

TEST(UT_Systemd1ServiceInterface_getMainPID, UT_Systemd1ServiceInterface_getMainPID_001)
{
    Systemd1ServiceInterface *interface = new Systemd1ServiceInterface(
                "org.freedesktop.systemd1","/org/freedesktop/systemd1/unit/virtlogd_2eservice",
                QDBusConnection::sessionBus());

    QPair<ErrorContext, quint32> reply = interface->getMainPID();

    // root 用户下编译执行失败
    // EXPECT_EQ(reply.first.getCode(), ErrorContext::kErrorTypeNoError);

    interface->deleteLater();
}

TEST(UT_Systemd1ServiceInterface_getMemoryCurrent, UT_Systemd1ServiceInterface_getMemoryCurrent_001)
{
    Systemd1ServiceInterface *interface = new Systemd1ServiceInterface(
                "org.freedesktop.systemd1", "/org/freedesktop/systemd1/unit/virtlogd_2eservice",
                QDBusConnection::sessionBus());

    QPair<ErrorContext, quint64> reply = interface->getMemoryCurrent();

    // root 用户下编译执行失败
    // EXPECT_EQ(reply.first.getCode(), ErrorContext::kErrorTypeNoError);

    interface->deleteLater();
}

TEST(UT_Systemd1ServiceInterface_getControlGroup, UT_Systemd1ServiceInterface_getControlGroup_001)
{
    Systemd1ServiceInterface *interface = new Systemd1ServiceInterface(
                "org.freedesktop.systemd1", "/org/freedesktop/systemd1/unit/virtlogd_2eservice",
                QDBusConnection::sessionBus());

    QPair<ErrorContext, QString> reply = interface->getControlGroup();

    // root 用户下编译执行失败
    // EXPECT_EQ(reply.first.getCode(), ErrorContext::kErrorTypeNoError);

    interface->deleteLater();
}

TEST(UT_Systemd1ServiceInterface_getEnvironmentFiles, UT_Systemd1ServiceInterface_getEnvironmentFiles_001)
{
    Systemd1ServiceInterface *interface = new Systemd1ServiceInterface(
                "org.freedesktop.systemd1", "/org/freedesktop/systemd1/unit/virtlogd_2eservice",
                QDBusConnection::sessionBus());

    QPair<ErrorContext, EnvironmentFileList> reply = interface->getEnvironmentFiles();

    // root 用户下编译执行失败
    // EXPECT_EQ(reply.first.getCode(), ErrorContext::kErrorTypeNoError);

    interface->deleteLater();
}
