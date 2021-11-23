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
#include "system/mem.h"

//gtest
#include "stub.h"
#include <gtest/gtest.h>

using namespace core::system;

class UT_MemInfo: public ::testing::Test
{
public:
    UT_MemInfo() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        m_tester = new MemInfo();
    }

    virtual void TearDown()
    {
        if (m_tester) {
            delete m_tester;
            m_tester = nullptr;
        }
    }

protected:
    MemInfo *m_tester;
};

TEST_F(UT_MemInfo, initTest)
{
}

TEST_F(UT_MemInfo, test_memTotal)
{
    m_tester->readMemInfo();
    EXPECT_NE(m_tester->memTotal(), 0);
}

TEST_F(UT_MemInfo, test_memAvailable)
{
    m_tester->readMemInfo();
    EXPECT_NE(m_tester->memAvailable(), 0);
}

TEST_F(UT_MemInfo, test_buffers)
{
    m_tester->readMemInfo();
    EXPECT_NE(m_tester->buffers(), 0);
}

TEST_F(UT_MemInfo, test_cached)
{
    m_tester->readMemInfo();
    EXPECT_NE(m_tester->cached(), 0);
}

TEST_F(UT_MemInfo, test_active)
{
    m_tester->readMemInfo();
    EXPECT_NE(m_tester->active(), 0);
}

TEST_F(UT_MemInfo, test_inactive)
{
    m_tester->readMemInfo();
    EXPECT_NE(m_tester->inactive(), 0);
}

TEST_F(UT_MemInfo, test_swapTotal)
{
    m_tester->readMemInfo();
    EXPECT_NE(m_tester->swapTotal(), 0);
}

TEST_F(UT_MemInfo, test_swapFree)
{
    m_tester->readMemInfo();
    EXPECT_NE(m_tester->swapFree(), 0);
}

TEST_F(UT_MemInfo, test_swapCached)
{
    m_tester->readMemInfo();
}

TEST_F(UT_MemInfo, test_shmem)
{
    m_tester->readMemInfo();
    EXPECT_NE(m_tester->shmem(), 0);
}

TEST_F(UT_MemInfo, test_slab)
{
    m_tester->readMemInfo();
    EXPECT_NE(m_tester->slab(), 0);
}

TEST_F(UT_MemInfo, test_dirty)
{
    m_tester->readMemInfo();
    EXPECT_NE(m_tester->dirty(), 0);
}

TEST_F(UT_MemInfo, test_mapped)
{
    m_tester->readMemInfo();
    EXPECT_NE(m_tester->mapped(), 0);
}

TEST_F(UT_MemInfo, test_readMemInfo)
{
    m_tester->readMemInfo();
    EXPECT_NE(m_tester->memTotal(), 0);
}
