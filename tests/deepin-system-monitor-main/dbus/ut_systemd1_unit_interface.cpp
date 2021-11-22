/*
* Copyright (C) 2019 ~ 2021 Uniontech Software Technology Co.,Ltd
*
* Author:     xuezifan<xuezifan@uniontech.com>
*
* Maintainer: xuezifan<xuezifan@uniontech.com>
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

//self
#include "dbus/systemd1_unit_interface.h"
#include "dbus/dbus_properties_interface.h"
#include "dbus/systemd1_unit_interface.h"
#include "dbus/dbus_common.h"
using namespace dbus::common;

//qt
#include <QByteArray>
#include <QList>
#include <QMap>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QtDBus>

//gtest
#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>

/***************************************STUB begin*********************************************/

/***************************************STUB end**********************************************/

class UT_Systemd1UnitInterface : public ::testing::Test
{
public:
    UT_Systemd1UnitInterface() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        m_tester = new Systemd1UnitInterface("org.freedesktop.systemd1", "/org/freedesktop/systemd1/unit/virtlogd_2eservice",
                                             QDBusConnection::sessionBus());
    }

    virtual void TearDown()
    {
        delete m_tester;
    }

protected:
    Systemd1UnitInterface *m_tester;
};

TEST_F(UT_Systemd1UnitInterface, initTest)
{

}

TEST_F(UT_Systemd1UnitInterface, test_getId_01)
{
    QPair<ErrorContext, QString> reply = m_tester->getId();
}

TEST_F(UT_Systemd1UnitInterface, test_getLoadState_01)
{
    QPair<ErrorContext, QString> reply = m_tester->getLoadState();
}

TEST_F(UT_Systemd1UnitInterface, test_getActiveState_01)
{
    QPair<ErrorContext, QString> reply = m_tester->getActiveState();
}

TEST_F(UT_Systemd1UnitInterface, test_getSubState_01)
{
    QPair<ErrorContext, QString> reply = m_tester->getSubState();
}

TEST_F(UT_Systemd1UnitInterface, test_getDescription_01)
{
    QPair<ErrorContext, QString> reply = m_tester->getDescription();
}

TEST_F(UT_Systemd1UnitInterface, test_canReload_01)
{
    QPair<ErrorContext, bool> reply = m_tester->canReload();
}

TEST_F(UT_Systemd1UnitInterface, test_canStart_001)
{
    QPair<ErrorContext, bool> reply = m_tester->canStart();
}

TEST_F(UT_Systemd1UnitInterface, test_canStop_01)
{
    QPair<ErrorContext, bool> reply = m_tester->canStop();
}
