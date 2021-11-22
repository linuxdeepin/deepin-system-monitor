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
#include "dbus/dbus_common.h"
#include "dbus/dbus_properties_interface.h"
#include "dbus/environment_file.h"
#include "dbus/systemd1_service_interface.h"

//qt
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
#include <QObject>
#include <QTest>

//gtest
#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>

/***************************************STUB begin*********************************************/

/***************************************STUB end**********************************************/

class UT_Systemd1ServiceInterface : public ::testing::Test
{
public:
    UT_Systemd1ServiceInterface() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        m_tester = new Systemd1ServiceInterface("org.freedesktop.systemd1", "/org/freedesktop/systemd1/unit/virtlogd_2eservice",
                                                QDBusConnection::sessionBus());
    }

    virtual void TearDown()
    {
        delete m_tester;
    }

protected:
    Systemd1ServiceInterface *m_tester;
};

TEST_F(UT_Systemd1ServiceInterface, initTest)
{

}

TEST_F(UT_Systemd1ServiceInterface, test_getMainPID_01)
{
    QPair<ErrorContext, quint32> reply = m_tester->getMainPID();

}

TEST_F(UT_Systemd1ServiceInterface, test_getMemoryCurrent_01)
{
    QPair<ErrorContext, quint64> reply = m_tester->getMemoryCurrent();
}

TEST_F(UT_Systemd1ServiceInterface, test_getControlGroup_01)
{
    QPair<ErrorContext, QString> reply = m_tester->getControlGroup();

}

TEST_F(UT_Systemd1ServiceInterface, test_getEnvironmentFiles_01)
{
    QPair<ErrorContext, EnvironmentFileList> reply = m_tester->getEnvironmentFiles();
}
