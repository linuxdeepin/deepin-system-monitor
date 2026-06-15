// Copyright (C) 2019 ~ 2021 Uniontech Software Technology Co.,Ltd.
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

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

// isDesktopEntryApp：注入 pid 到 m_desktopEntryCache 后返回 true，否则 false
TEST_F(UT_WMWindowList, test_isDesktopEntryApp_001)
{
    pid_t pid = 5678;
    EXPECT_FALSE(m_tester->isDesktopEntryApp(pid));

    m_tester->m_desktopEntryCache << pid;
    EXPECT_TRUE(m_tester->isDesktopEntryApp(pid));
}

// isTrayApp：注入 pid 到 m_trayAppcache 后返回 true，否则 false
TEST_F(UT_WMWindowList, test_isTrayApp_001)
{
    pid_t pid = 6789;
    EXPECT_FALSE(m_tester->isTrayApp(pid));

    m_tester->m_trayAppcache[pid] = std::make_unique<wm_window_t>();
    EXPECT_TRUE(m_tester->isTrayApp(pid));
}

// isGuiApp：注入 pid 到 m_guiAppcache 后返回 true，否则 false
TEST_F(UT_WMWindowList, test_isGuiApp_001)
{
    pid_t pid = 7890;
    EXPECT_FALSE(m_tester->isGuiApp(pid));

    m_tester->m_guiAppcache[pid] = std::make_unique<wm_window_t>();
    EXPECT_TRUE(m_tester->isGuiApp(pid));
}

// removeDesktopEntryApp：命中分支（先注入再移除）
TEST_F(UT_WMWindowList, test_removeDesktopEntryApp_002)
{
    pid_t pid = 8901;
    m_tester->m_desktopEntryCache << pid;
    ASSERT_TRUE(m_tester->isDesktopEntryApp(pid));

    m_tester->removeDesktopEntryApp(pid);
    EXPECT_FALSE(m_tester->isDesktopEntryApp(pid));
}
