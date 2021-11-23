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
#include "system/netif_info_db.h"

//gtest
#include "stub.h"
#include <gtest/gtest.h>

using namespace core::system;

class UT_NetifInfoDB: public ::testing::Test
{
public:
    UT_NetifInfoDB() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        m_tester = new NetifInfoDB();
    }

    virtual void TearDown()
    {
        if (m_tester) {
            delete m_tester;
            m_tester = nullptr;
        }
    }

protected:
    NetifInfoDB *m_tester;
};

TEST_F(UT_NetifInfoDB, initTest)
{
}

TEST_F(UT_NetifInfoDB, test_update_addr)
{
    m_tester->update_addr();
    EXPECT_TRUE( m_tester->m_addrIpv4DB.size() > 0);
}

TEST_F(UT_NetifInfoDB, test_update_netif_info)
{
    m_tester->update_netif_info();
    EXPECT_TRUE( m_tester->m_infoDB.size() > 0);
}

TEST_F(UT_NetifInfoDB, test_update)
{
    m_tester->update();
    EXPECT_TRUE( m_tester->m_infoDB.size() > 0);

}
