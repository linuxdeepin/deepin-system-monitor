/*
* Copyright (C) 2019 ~ 2021 Uniontech Software Technology Co.,Ltd.
*
* Author:     lishiqi <lishiqi@uniontech.com>
*
* Maintainer: lishiqi  <lishiqi@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
//self
#include "process/process_icon_cache.h"
#include "process/process_icon.h"
#include "process/process.h"
//gtest
#include "stub.h"
#include <gtest/gtest.h>

using namespace core::process;

class UT_ProcessIconCache : public ::testing::Test
{
public:
    UT_ProcessIconCache() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        m_tester = new ProcessIconCache(nullptr);
    }

    virtual void TearDown()
    {
        if(m_tester){
            delete m_tester;
            m_tester = nullptr;
        }
    }

protected:
    ProcessIconCache *m_tester;
};

TEST_F(UT_ProcessIconCache, initTest)
{


}

TEST_F(UT_ProcessIconCache, test_instance_001)
{
    m_tester->instance();

}

TEST_F(UT_ProcessIconCache, test_getProcessIcon_001)
{
    pid_t pid =1000;
    m_tester->getProcessIcon(pid);

}

TEST_F(UT_ProcessIconCache, test_addProcessIcon_001)
{
    pid_t pid =1000;
    ProcessIcon *icon = new ProcessIcon();
    m_tester->addProcessIcon(pid,icon);

}
