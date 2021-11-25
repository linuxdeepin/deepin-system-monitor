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
#include "wm/wm_window_list.h"
#include "wm/wm_info.h"
#include "process/process_db.h"
//gtest
#include "stub.h"
#include <gtest/gtest.h>
//Qt
using namespace core::process;
using namespace core::wm;

/***********************************************************Stub bdgin**************************************************/
/***********************************************************Stub end**************************************************/
class UT_WMWindowList: public ::testing::Test
{
public:
    UT_WMWindowList() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        m_tester = new WMWindowList();
    }

    virtual void TearDown()
    {
        if(m_tester){
            delete m_tester;
            m_tester=nullptr;
        }
    }

protected:
    WMWindowList *m_tester;
};

TEST_F(UT_WMWindowList, initTest)
{

}

TEST_F(UT_WMWindowList, test_addDesktopEntryApp_001)
{
    Process *proc = new Process();
    m_tester->addDesktopEntryApp(proc);
    delete proc;
}

TEST_F(UT_WMWindowList, test_getAppCount_001)
{

    m_tester->getAppCount();
}

TEST_F(UT_WMWindowList, test_removeDesktopEntryApp_001)
{

    m_tester->removeDesktopEntryApp(100000);
}

TEST_F(UT_WMWindowList, test_getWindowIcon_001)
{
    m_tester->getWindowIcon(100000);
}

TEST_F(UT_WMWindowList, test_getWindowPid_001)
{
    m_tester->getWindowPid(1000);
}
