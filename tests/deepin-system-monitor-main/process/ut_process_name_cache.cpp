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
#include "process/process_name_cache.h"
//gtest
#include "stub.h"
#include <gtest/gtest.h>
//Qt
#include <QFileInfo>
#include <QApplication>
#include <QCache>
using namespace core::process;

typedef QCache<pid_t, ProcessName> myCache;
/***************************************STUB begin*********************************************/
void stub_insert_insert(){
    return;
}
/***************************************STUB end**********************************************/
class UT_ProcessNameCache : public ::testing::Test
{
public:
    UT_ProcessNameCache() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        m_tester = new ProcessNameCache(nullptr);
    }

    virtual void TearDown()
    {
        delete m_tester;
    }

protected:
    ProcessNameCache *m_tester;
};

TEST_F(UT_ProcessNameCache, initTest)
{

}

TEST_F(UT_ProcessNameCache, test_instance_001)
{
    m_tester->instance();
}

TEST_F(UT_ProcessNameCache, test_insert_001)
{
    ProcessName *name = new ProcessName;
    Stub b;
    b.set(ADDR(myCache,insert),stub_insert_insert);
    m_tester->insert(1000,name);
    delete name;
}

TEST_F(UT_ProcessNameCache, test_remove_001)
{
    m_tester->remove(1000);
}


TEST_F(UT_ProcessNameCache, test_clear_001)
{
    m_tester->clear();
}

TEST_F(UT_ProcessNameCache, test_processName_001)
{
    m_tester->processName(1000);
}

TEST_F(UT_ProcessNameCache, test_contains_001)
{
    m_tester->contains(1000);
}
