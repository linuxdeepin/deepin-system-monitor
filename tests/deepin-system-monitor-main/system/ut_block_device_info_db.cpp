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
#include "system/block_device_info_db.h"

//gtest
#include "stub.h"
#include <gtest/gtest.h>

//qt
#include <QString>

using namespace core::system;

/***************************************STUB begin*********************************************/

//void stub_ReadDeviceInfo()
//{
//    core::system::BlockDevice blockDevice;
//}

/***************************************STUB end**********************************************/

class UT_BlockDeviceInfoDB: public ::testing::Test
{
public:
    UT_BlockDeviceInfoDB() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        m_tester = new BlockDeviceInfoDB();
    }

    virtual void TearDown()
    {
        if (m_tester) {
            delete m_tester;
            m_tester = nullptr;
        }
    }

protected:
    BlockDeviceInfoDB *m_tester;
};

TEST_F(UT_BlockDeviceInfoDB, initTest)
{
}

TEST_F(UT_BlockDeviceInfoDB, test_readDiskInfo)
{
    m_tester->readDiskInfo();
    EXPECT_NE(m_tester->m_deviceList.size(), 0);
}

TEST_F(UT_BlockDeviceInfoDB, test_update)
{
    m_tester->readDiskInfo();
    EXPECT_NE(m_tester->m_deviceList.size(), 0);
}
