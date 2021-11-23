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
#include "system/wireless.h"

//gtest
#include "stub.h"
#include <gtest/gtest.h>

using namespace core::system;

class UT_wireless: public ::testing::Test
{
public:
    UT_wireless() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        m_tester = new wireless();
    }

    virtual void TearDown()
    {
        if (m_tester) {
            delete m_tester;
            m_tester = nullptr;
        }
    }

protected:
    wireless *m_tester;
};

TEST_F(UT_wireless, initTest)
{
}

TEST_F(UT_wireless, test_essid)
{
    m_tester->essid();
}

TEST_F(UT_wireless, test_link_quality)
{
    m_tester->link_quality();
}

TEST_F(UT_wireless, test_signal_levle)
{
    m_tester->signal_levle();
}

TEST_F(UT_wireless, test_noise_level)
{
    m_tester->noise_level();
}

TEST_F(UT_wireless, test_is_wireless)
{
    m_tester->m_bwireless = true;
    EXPECT_EQ(m_tester->is_wireless(), true);
}

TEST_F(UT_wireless, test_read_wireless_info)
{
    m_tester->read_wireless_info();
}
