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
#include "system/net_info.h"
#include <QDebug>

//gtest
#include "stub.h"
#include <gtest/gtest.h>

using namespace core::system;

class UT_NetInfo: public ::testing::Test
{
public:
    UT_NetInfo() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        m_tester = new NetInfo();
    }

    virtual void TearDown()
    {
        if (m_tester) {
            delete m_tester;
            m_tester = nullptr;
        }
    }

protected:
    NetInfo *m_tester;
};

TEST_F(UT_NetInfo, initTest)
{
}

TEST_F(UT_NetInfo, test_recvBps)
{
    m_tester->resdNetInfo();
    EXPECT_NE(m_tester->recvBps(), 0);
}

TEST_F(UT_NetInfo, test_sentBps)
{
    m_tester->resdNetInfo();
    EXPECT_NE(m_tester->sentBps(), 0);
}

TEST_F(UT_NetInfo, test_totalRecvBytes)
{
    m_tester->resdNetInfo();
    sleep(1);
    m_tester->resdNetInfo();
    EXPECT_NE(m_tester->totalRecvBytes(), 0);
}

TEST_F(UT_NetInfo, test_totalSentBytes)
{
    m_tester->resdNetInfo();
    sleep(1);
    m_tester->resdNetInfo();
    EXPECT_NE(m_tester->totalSentBytes(), 0);
//    qInfo()<<"totalSentBytes: "<<m_tester->totalSentBytes();
}

TEST_F(UT_NetInfo, test_resdNetInfo)
{
    m_tester->resdNetInfo();
    sleep(1);
    m_tester->resdNetInfo();
    EXPECT_NE(m_tester->totalSentBytes(), 0);
}

