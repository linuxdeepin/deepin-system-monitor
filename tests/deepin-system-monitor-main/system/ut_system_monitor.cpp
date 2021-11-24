/*
* Copyright (C) 2019 ~ 2021 Uniontech Software Technology Co.,Ltd
*
* Author:      baohaifeng <baohaifeng@uniontech.com>
* Maintainer:  baohaifeng <baohaifeng@uniontech.com>
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
#include "system/system_monitor.h"
#include "process/process_set.h"
//gtest
#include "stub.h"
#include <gtest/gtest.h>

//qt
#include <QTimerEvent>

using namespace core::system;

class UT_SystemMonitor: public ::testing::Test
{
public:
    UT_SystemMonitor() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        m_tester = new SystemMonitor();
    }

    virtual void TearDown()
    {
        if (m_tester) {
            delete m_tester;
            m_tester = nullptr;
        }
    }

protected:
    SystemMonitor *m_tester;
};

TEST_F(UT_SystemMonitor, initTest)
{
}

TEST_F(UT_SystemMonitor, test_instance)
{
    EXPECT_TRUE(m_tester->instance() != nullptr);
}

TEST_F(UT_SystemMonitor, test_deviceDB)
{
    EXPECT_TRUE(m_tester->deviceDB() != nullptr);
}

TEST_F(UT_SystemMonitor, test_processDB)
{
    EXPECT_TRUE(m_tester->processDB() != nullptr);
}

TEST_F(UT_SystemMonitor, test_sysInfo)
{
    EXPECT_TRUE(m_tester->sysInfo() != nullptr);
}

TEST_F(UT_SystemMonitor, test_startMonitorJob)
{
    m_tester->startMonitorJob();
    EXPECT_TRUE(m_tester->m_basictimer.isActive() == true);
}

TEST_F(UT_SystemMonitor, test_timerEvent)
{
    QTimerEvent event(1);
    m_tester->timerEvent(&event);
}
