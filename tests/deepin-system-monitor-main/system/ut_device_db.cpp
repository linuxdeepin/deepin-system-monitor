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
#include "system/device_db.h"
#include "system/netif_info_db.h"
#include "system/block_device_info_db.h"
#include "system/mem.h"
#include "system/cpu_set.h"
#include "system/diskio_info.h"
#include "system/net_info.h"

//gtest
#include "stub.h"
#include <gtest/gtest.h>

using namespace core::system;

class UT_DeviceDB: public ::testing::Test
{
public:
    UT_DeviceDB() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        m_tester = new DeviceDB();
    }

    virtual void TearDown()
    {
        if (m_tester) {
            delete m_tester;
            m_tester = nullptr;
        }
    }

protected:
    DeviceDB *m_tester;
};

TEST_F(UT_DeviceDB, initTest)
{
}

TEST_F(UT_DeviceDB, test_update)
{
    m_tester->update();
    sleep(2);
    m_tester->update();

    EXPECT_TRUE(m_tester->netInfo()->totalRecvBytes() != 0 || m_tester->netInfo()->totalSentBytes() != 0);
}

TEST_F(UT_DeviceDB, test_instance)
{
    EXPECT_TRUE(m_tester->instance() != nullptr);
}

TEST_F(UT_DeviceDB, test_blockDeviceInfoDB)
{
    m_tester->update();
    EXPECT_NE(m_tester->blockDeviceInfoDB()->deviceList().size(), 0);
}

TEST_F(UT_DeviceDB, test_netifInfoDB)
{
}

TEST_F(UT_DeviceDB, test_memInfo)
{
    m_tester->update();
    EXPECT_NE(m_tester->memInfo()->memTotal(), 0);
}

TEST_F(UT_DeviceDB, test_cpuSet)
{
    m_tester->update();
    EXPECT_NE(m_tester->cpuSet()->cpuCount(), 0);
}

TEST_F(UT_DeviceDB, test_diskIoInfo)
{
    m_tester->update();
    EXPECT_TRUE(m_tester->diskIoInfo()->m_readBps != 0 || m_tester->diskIoInfo()->m_writeBps != 0);
}

TEST_F(UT_DeviceDB, test_netInfo)
{
    m_tester->update();
    sleep(2);
    m_tester->update();

//    qInfo()<<"totalRecvBytes:"<<m_tester->netInfo()->totalRecvBytes();
//    qInfo()<<"totalSentBytes:"<<m_tester->netInfo()->totalSentBytes();
    EXPECT_TRUE(m_tester->netInfo()->totalRecvBytes() != 0 || m_tester->netInfo()->totalSentBytes() != 0);
}


