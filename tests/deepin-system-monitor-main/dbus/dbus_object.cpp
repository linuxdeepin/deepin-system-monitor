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
#include "dbus/dbus_object.h"
#include "gui/main_window.h"
#include "application.h"

//qt
#include <QDBusConnection>
#include <QDebug>

#define DBUS_SERVER             "com.deepin.systemMonitor"
#define DBUS_SERVER_PATH        "/com/deepin/systemMonitor"

//gtest
#include "stub.h"
#include <gtest/gtest.h>

/***************************************STUB begin*********************************************/

/***************************************STUB end**********************************************/

class UT_DbusObject : public ::testing::Test
{
public:
    UT_DbusObject() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        m_tester = new DBusObject();
    }

    virtual void TearDown()
    {
        if (m_tester) {
            delete m_tester;
            m_tester = nullptr;
        }
    }

protected:
    DBusObject *m_tester;
};

TEST_F(UT_DbusObject, initTest)
{

}

TEST_F(UT_DbusObject, test_getInstance_01)
{
    m_tester->getInstance();
}

TEST_F(UT_DbusObject, test_registerOrNotify_01)
{
    m_tester->registerOrNotify();
}

TEST_F(UT_DbusObject, test_unRegister_01)
{
    m_tester->unRegister();
}





