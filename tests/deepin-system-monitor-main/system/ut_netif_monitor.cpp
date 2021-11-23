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
#include "system/netif_monitor.h"


//gtest
#include "stub.h"
#include <gtest/gtest.h>

//qt
#include <QDebug>
#include <QTimer>
using namespace core::system;

#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>


class UT_NetifMonitor: public QObject, public ::testing::Test
{
//    Q_OBJECT
public:
    UT_NetifMonitor() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        m_tester = new NetifMonitor();
    }

    virtual void TearDown()
    {
        if (m_tester) {
            delete m_tester;
            m_tester = nullptr;
        }
    }

protected:
    NetifMonitor *m_tester;
};

TEST_F(UT_NetifMonitor, initTest)
{
}

TEST_F(UT_NetifMonitor, test_instance)
{
    EXPECT_TRUE(m_tester->instance() != nullptr);
}

TEST_F(UT_NetifMonitor, test_startNetmonitorJob)
{
    m_tester->startNetmonitorJob();
    EXPECT_TRUE( m_tester->m_packetMonitorThread.isRunning() == true);
}

TEST_F(UT_NetifMonitor, test_handleNetData)
{
    QTimer::singleShot(100, this, [=]() {
        m_tester->handleNetData();
    });
}
