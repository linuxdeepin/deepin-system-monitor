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
#include "system/netlink.h"

//gtest
#include "stub.h"
#include <gtest/gtest.h>

using namespace core::system;

class UT_Netlink: public ::testing::Test
{
public:
    UT_Netlink() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        m_tester = new Netlink();
    }

    virtual void TearDown()
    {
        if (m_tester) {
            delete m_tester;
            m_tester = nullptr;
        }
    }

protected:
    Netlink *m_tester;
};

TEST_F(UT_Netlink, initTest)
{
}

TEST_F(UT_Netlink, test_linkIterator)
{
    EXPECT_TRUE(m_tester->linkIterator().hasNext() == true);
}

TEST_F(UT_Netlink, test_addrIterator)
{
    EXPECT_TRUE(m_tester->addrIterator().hasNext() == true);
}


