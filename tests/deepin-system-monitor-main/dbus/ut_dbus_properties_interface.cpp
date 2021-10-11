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

#include "common/error_context.h"
#include "dbus/dbus_properties_interface.h"

#include <QByteArray>
#include <QList>
#include <QMap>
#include <QObject>
#include <QString>
#include <QVariant>
#include <QtDBus>

#include "ut_dbus_properties_interface.h"

Ut_DBusPropertiesInterface::Ut_DBusPropertiesInterface()
{
}

QPair<ErrorContext, QDBusMessage> DBusPropertiesInterface_Get_FontSize_stub()
{
    ErrorContext cxt;
    QDBusMessage msg;
    cxt.setCode(ErrorContext::kErrorTypeNoError);
    return QPair<ErrorContext, QDBusMessage>(cxt, msg.createReply(15));
}

TEST(UT_DBusPropertiesInterface_Get, UT_DBusPropertiesInterface_Get_001)
{
    DBusPropertiesInterface *interface = new DBusPropertiesInterface("com.deepin.daemon.Appearance",
                                                                     "/com/deepin/daemon/Appearance",
                                                                     QDBusConnection::sessionBus(), nullptr);
    Stub stub;
    stub.set(ADDR(DBusPropertiesInterface, Get), DBusPropertiesInterface_Get_FontSize_stub);

    QPair<ErrorContext, QDBusMessage> reply = interface->Get("com.deepin.daemon.Appearance", "FontSize");

    EXPECT_EQ(reply.first.getCode(), ErrorContext::kErrorTypeNoError);
    EXPECT_GT(reply.second.arguments().first().toDouble(), 0.0);

    interface->deleteLater();
}
