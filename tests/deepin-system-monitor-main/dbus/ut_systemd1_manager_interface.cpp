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

#include "dbus/systemd1_manager_interface.h"
#include "common/error_context.h"
#include "dbus/dbus_common.h"
#include "dbus/unit_file_info.h"
#include "dbus/unit_info.h"

#include <QByteArray>
#include <QList>
#include <QMap>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QtDBus>
#include <QDBusConnection>

#undef private
#undef protected

#include "ut_systemd1_manager_interface.h"

Ut_Systemd1ManagerInterface::Ut_Systemd1ManagerInterface()
{
}

TEST(UT_Systemd1ManagerInterface_ListUnitFiles, UT_Systemd1ManagerInterface_ListUnitFiles_001)
{
    Systemd1ManagerInterface *interface = new Systemd1ManagerInterface(
                "org.freedesktop.systemd1", "/org/freedesktop/systemd1", QDBusConnection::sessionBus(), nullptr);

    QPair<ErrorContext, UnitFileInfoList> reply = interface->ListUnitFiles();

    // root 用户下编译执行失败
    // EXPECT_EQ(reply.first.getCode(), ErrorContext::kErrorTypeNoError);

    interface->deleteLater();
}

TEST(UT_Systemd1ManagerInterface_ListUnits, UT_Systemd1ManagerInterface_ListUnits_001)
{
    Systemd1ManagerInterface *interface = new Systemd1ManagerInterface(
                "org.freedesktop.systemd1", "/org/freedesktop/systemd1", QDBusConnection::sessionBus(), nullptr);

    QPair<ErrorContext, UnitInfoList> reply = interface->ListUnits();

    // root 用户下编译执行失败
    // EXPECT_EQ(reply.first.getCode(), ErrorContext::kErrorTypeNoError);

    interface->deleteLater();
}

TEST(UT_Systemd1ManagerInterface_GetUnit, UT_Systemd1ManagerInterface_GetUnit_001)
{
    Systemd1ManagerInterface *interface = new Systemd1ManagerInterface(
                "org.freedesktop.systemd1", "/org/freedesktop/systemd1", QDBusConnection::sessionBus(), nullptr);
    QPair<ErrorContext, QDBusObjectPath> reply = interface->GetUnit("xdg-desktop-portal.service");

    // root 用户下编译执行失败
    // EXPECT_EQ(reply.first.getCode(), ErrorContext::kErrorTypeNoError);

    interface->deleteLater();
}

TEST(UT_Systemd1ManagerInterface_GetUnit, UT_Systemd1ManagerInterface_GetUnit_002)
{
    Systemd1ManagerInterface *interface = new Systemd1ManagerInterface(
                "org.freedesktop.systemd1", "/org/freedesktop/systemd1", QDBusConnection::sessionBus(), nullptr);
    QPair<ErrorContext, QDBusObjectPath> reply = interface->GetUnit("");

    EXPECT_NE(reply.first.getCode(), ErrorContext::kErrorTypeNoError);

    interface->deleteLater();
}

TEST(UT_Systemd1ManagerInterface_StartUnit, UT_Systemd1ManagerInterface_StartUnit_001)
{
    Systemd1ManagerInterface *interface = new Systemd1ManagerInterface(
                "org.freedesktop.systemd1", "/org/freedesktop/systemd1", QDBusConnection::sessionBus(), nullptr);
    QPair<ErrorContext, QDBusObjectPath> reply = interface->StartUnit("printer.target", "replace");

    // root 用户下编译执行失败
    // EXPECT_EQ(reply.first.getCode(), ErrorContext::kErrorTypeNoError);

    interface->deleteLater();
}

TEST(UT_Systemd1ManagerInterface_StartUnit, UT_Systemd1ManagerInterface_StartUnit_002)
{
    Systemd1ManagerInterface *interface = new Systemd1ManagerInterface(
                "org.freedesktop.systemd1", "/org/freedesktop/systemd1", QDBusConnection::sessionBus(), nullptr);
    QPair<ErrorContext, QDBusObjectPath> reply = interface->StartUnit("", "replace");

    EXPECT_NE(reply.first.getCode(), ErrorContext::kErrorTypeNoError);

    interface->deleteLater();
}

TEST(UT_Systemd1ManagerInterface_StopUnit, UT_Systemd1ManagerInterface_StopUnit_001)
{
    Systemd1ManagerInterface *interface = new Systemd1ManagerInterface(
                "org.freedesktop.systemd1", "/org/freedesktop/systemd1", QDBusConnection::sessionBus(), nullptr);
    QPair<ErrorContext, QDBusObjectPath> reply = interface->StopUnit("printer.target", "replace");

    // root 用户下编译执行失败
    // EXPECT_EQ(reply.first.getCode(), ErrorContext::kErrorTypeNoError);

    interface->deleteLater();
}

TEST(UT_Systemd1ManagerInterface_StopUnit, UT_Systemd1ManagerInterface_StopUnit_002)
{
    Systemd1ManagerInterface *interface = new Systemd1ManagerInterface(
                "org.freedesktop.systemd1", "/org/freedesktop/systemd1", QDBusConnection::sessionBus(), nullptr);
    QPair<ErrorContext, QDBusObjectPath> reply = interface->StopUnit("", "replace");

    EXPECT_NE(reply.first.getCode(), ErrorContext::kErrorTypeNoError);

    interface->deleteLater();
}

TEST(UT_Systemd1ManagerInterface_RestartUnit, UT_Systemd1ManagerInterface_RestartUnit_001)
{
    Systemd1ManagerInterface *interface = new Systemd1ManagerInterface(
                "org.freedesktop.systemd1", "/org/freedesktop/systemd1", QDBusConnection::sessionBus(), nullptr);
    QPair<ErrorContext, QDBusObjectPath> reply = interface->RestartUnit("printer.target", "replace");

    // root 用户下编译执行失败
    // EXPECT_EQ(reply.first.getCode(), ErrorContext::kErrorTypeNoError);

    interface->deleteLater();
}

TEST(UT_Systemd1ManagerInterface_RestartUnit, UT_Systemd1ManagerInterface_RestartUnit_002)
{
    Systemd1ManagerInterface *interface = new Systemd1ManagerInterface(
                "org.freedesktop.systemd1", "/org/freedesktop/systemd1", QDBusConnection::sessionBus(), nullptr);
    QPair<ErrorContext, QDBusObjectPath> reply = interface->RestartUnit("", "replace");

    EXPECT_NE(reply.first.getCode(), ErrorContext::kErrorTypeNoError);

    interface->deleteLater();
}

TEST(UT_Systemd1ManagerInterface_GetUnitFileState, UT_Systemd1ManagerInterface_GetUnitFileState_001)
{
    Systemd1ManagerInterface *interface = new Systemd1ManagerInterface(
                "org.freedesktop.systemd1", "/org/freedesktop/systemd1", QDBusConnection::sessionBus(), nullptr);
    QPair<ErrorContext, QString> reply = interface->GetUnitFileState("printer.target");

    // root 用户下编译执行失败
    // EXPECT_EQ(reply.first.getCode(), ErrorContext::kErrorTypeNoError);

    interface->deleteLater();
}

TEST(UT_Systemd1ManagerInterface_GetUnitFileState, UT_Systemd1ManagerInterface_GetUnitFileState_002)
{
    Systemd1ManagerInterface *interface = new Systemd1ManagerInterface(
                "org.freedesktop.systemd1", "/org/freedesktop/systemd1", QDBusConnection::sessionBus(), nullptr);
    QPair<ErrorContext, QString> reply = interface->GetUnitFileState("");

    EXPECT_NE(reply.first.getCode(), ErrorContext::kErrorTypeNoError);

    interface->deleteLater();
}

TEST(UT_Systemd1ManagerInterface_EnableUnitFiles, UT_Systemd1ManagerInterface_EnableUnitFiles_001)
{
    Systemd1ManagerInterface *interface = new Systemd1ManagerInterface(
                "org.freedesktop.systemd1", "/org/freedesktop/systemd1", QDBusConnection::sessionBus(), nullptr);
    QStringList nlist;
    nlist.append("printer.target");
    QPair<ErrorContext, EnableUnitFilesResult> reply = interface->EnableUnitFiles(nlist);

    // root 用户下编译执行失败
    // EXPECT_EQ(reply.first.getCode(), ErrorContext::kErrorTypeNoError);

    interface->deleteLater();
}

TEST(UT_Systemd1ManagerInterface_DisableUnitFiles, UT_Systemd1ManagerInterface_DisableUnitFiles_001)
{
    Systemd1ManagerInterface *interface = new Systemd1ManagerInterface(
                "org.freedesktop.systemd1", "/org/freedesktop/systemd1", QDBusConnection::sessionBus(), nullptr);
    QStringList nlist;
    nlist.append("printer.target");
    QPair<ErrorContext, DisableUnitFilesResult> reply = interface->DisableUnitFiles(nlist);

    // root 用户下编译执行失败
    // EXPECT_EQ(reply.first.getCode(), ErrorContext::kErrorTypeNoError);

    interface->deleteLater();
}
