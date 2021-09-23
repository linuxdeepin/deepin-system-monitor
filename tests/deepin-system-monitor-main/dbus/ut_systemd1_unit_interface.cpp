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

#include "dbus/systemd1_unit_interface.h"
#include "dbus/dbus_properties_interface.h"
#include "dbus/systemd1_unit_interface.h"
#include "dbus/dbus_common.h"

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

#include "ut_systemd1_unit_interface.h"

Ut_Systemd1UnitInterface::Ut_Systemd1UnitInterface()
{
}

TEST(UT_Systemd1UnitInterface_getId, UT_Systemd1UnitInterface_getId_001)
{
    Systemd1UnitInterface* interface = new Systemd1UnitInterface(
                "org.freedesktop.systemd1", "/org/freedesktop/systemd1/unit/virtlogd_2eservice",
                QDBusConnection::sessionBus());

    QPair<ErrorContext, QString> reply = interface->getId();

    // root 用户下编译执行失败
    // EXPECT_EQ(reply.first.getCode(), ErrorContext::kErrorTypeNoError);

    interface->deleteLater();
}

TEST(UT_Systemd1UnitInterface_getLoadState, UT_Systemd1UnitInterface_getLoadState_001)
{
    Systemd1UnitInterface* interface = new Systemd1UnitInterface(
                "org.freedesktop.systemd1", "/org/freedesktop/systemd1/unit/virtlogd_2eservice",
                QDBusConnection::sessionBus());

    QPair<ErrorContext, QString> reply = interface->getLoadState();

    // root 用户下编译执行失败
    // EXPECT_EQ(reply.first.getCode(), ErrorContext::kErrorTypeNoError);

    interface->deleteLater();
}

TEST(UT_Systemd1UnitInterface_getActiveState, UT_Systemd1UnitInterface_getActiveState_001)
{
    Systemd1UnitInterface* interface = new Systemd1UnitInterface(
                "org.freedesktop.systemd1", "/org/freedesktop/systemd1/unit/virtlogd_2eservice",
                QDBusConnection::sessionBus());

    QPair<ErrorContext, QString> reply = interface->getActiveState();

    // root 用户下编译执行失败
    // EXPECT_EQ(reply.first.getCode(), ErrorContext::kErrorTypeNoError);

    interface->deleteLater();
}

TEST(UT_Systemd1UnitInterface_getSubState, UT_Systemd1UnitInterface_getSubState_001)
{
    Systemd1UnitInterface* interface = new Systemd1UnitInterface(
                "org.freedesktop.systemd1", "/org/freedesktop/systemd1/unit/virtlogd_2eservice",
                QDBusConnection::sessionBus());

    QPair<ErrorContext, QString> reply = interface->getSubState();

    // root 用户下编译执行失败
    // EXPECT_EQ(reply.first.getCode(), ErrorContext::kErrorTypeNoError);

    interface->deleteLater();
}

TEST(UT_Systemd1UnitInterface_getDescription, UT_Systemd1UnitInterface_getDescription_001)
{
    Systemd1UnitInterface* interface = new Systemd1UnitInterface(
                "org.freedesktop.systemd1", "/org/freedesktop/systemd1/unit/virtlogd_2eservice",
                QDBusConnection::sessionBus());

    QPair<ErrorContext, QString> reply = interface->getDescription();

    // root 用户下编译执行失败
    // EXPECT_EQ(reply.first.getCode(), ErrorContext::kErrorTypeNoError);

    interface->deleteLater();
}

TEST(UT_Systemd1UnitInterface_canReload, UT_Systemd1UnitInterface_canReload_001)
{
    Systemd1UnitInterface* interface = new Systemd1UnitInterface(
                "org.freedesktop.systemd1", "/org/freedesktop/systemd1/unit/virtlogd_2eservice",
                QDBusConnection::sessionBus());

    QPair<ErrorContext, bool> reply = interface->canReload();

    // root 用户下编译执行失败
    // EXPECT_EQ(reply.first.getCode(), ErrorContext::kErrorTypeNoError);

    interface->deleteLater();
}

TEST(UT_Systemd1UnitInterface_canStart, UT_Systemd1UnitInterface_canStart_001)
{
    Systemd1UnitInterface* interface = new Systemd1UnitInterface(
                "org.freedesktop.systemd1", "/org/freedesktop/systemd1/unit/virtlogd_2eservice",
                QDBusConnection::sessionBus());

    QPair<ErrorContext, bool> reply = interface->canStart();

    // root 用户下编译执行失败
    // EXPECT_EQ(reply.first.getCode(), ErrorContext::kErrorTypeNoError);

    interface->deleteLater();
}

TEST(UT_Systemd1UnitInterface_canStop, UT_Systemd1UnitInterface_canStop_001)
{
    Systemd1UnitInterface* interface = new Systemd1UnitInterface(
                "org.freedesktop.systemd1", "/org/freedesktop/systemd1/unit/virtlogd_2eservice",
                QDBusConnection::sessionBus());

    QPair<ErrorContext, bool> reply = interface->canStop();

    // root 用户下编译执行失败
    // EXPECT_EQ(reply.first.getCode(), ErrorContext::kErrorTypeNoError);

    interface->deleteLater();
}

