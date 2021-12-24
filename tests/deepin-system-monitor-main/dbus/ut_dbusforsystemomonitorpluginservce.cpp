/*
* Copyright (C) 2019 ~ 2021 Uniontech Software Technology Co.,Ltd
*
* Author:     yukuan<yukuan@uniontech.com>
*
* Maintainer: yukuan<yukuan@uniontech.com>
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
#include "dbus/dbusforsystemomonitorpluginservce.h"
#include "application.h"
//qt
#include <QList>
#include <QString>
#include <QtDBus>
#include <QDebug>

//gtest
#include "stub.h"
#include <gtest/gtest.h>

/***************************************STUB begin*********************************************/
Application::TaskState stub_getCurrentTaskState()
{
    return Application::TaskState::kTaskFinished;
}
/***************************************STUB end**********************************************/

class UT_DBusForSystemoMonitorPluginServce : public ::testing::Test
{
public:
    UT_DBusForSystemoMonitorPluginServce() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        static MainWindow mainwindow;
        gApp->setMainWindow(&mainwindow);
        m_tester = new DBusForSystemoMonitorPluginServce();
    }

    virtual void TearDown()
    {
        if (m_tester) {
            delete m_tester;
            m_tester = nullptr;
        }
    }

protected:
    DBusForSystemoMonitorPluginServce *m_tester;
};

TEST_F(UT_DBusForSystemoMonitorPluginServce, initTest)
{

}

TEST_F(UT_DBusForSystemoMonitorPluginServce, test_slotJumpDetailWidget_01)
{
    Stub stub;
    stub.set(ADDR(Application, getCurrentTaskState), stub_getCurrentTaskState);
    m_tester->slotJumpDetailWidget("MSG_CPU");
}

TEST_F(UT_DBusForSystemoMonitorPluginServce, test_slotJumpProcessWidget_01)
{
    Stub stub;
    stub.set(ADDR(Application, getCurrentTaskState), stub_getCurrentTaskState);
    m_tester->slotJumpProcessWidget("MSG_PROCESS");
}

TEST_F(UT_DBusForSystemoMonitorPluginServce, test_slotRaiseWindow_01)
{
    Stub stub;
    stub.set(ADDR(Application, getCurrentTaskState), stub_getCurrentTaskState);
    m_tester->slotRaiseWindow();
}
