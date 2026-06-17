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

// 扩展测试：覆盖更多方法
TEST_F(UT_WMWindowList, test_addDesktopEntryApp_multiple)
{
    // 测试添加多个进程
    Process *proc1 = new Process();
    Process *proc2 = new Process();
    m_tester->addDesktopEntryApp(proc1);
    m_tester->addDesktopEntryApp(proc2);
    delete proc1;
    delete proc2;
}

TEST_F(UT_WMWindowList, test_getAppCount_multiple)
{
    // 测试获取多个应用数量
    Process *proc = new Process();
    m_tester->addDesktopEntryApp(proc);
    m_tester->getAppCount();
    delete proc;
}

TEST_F(UT_WMWindowList, test_getWindowIcon_multiple)
{
    // 测试获取多个窗口图标
    m_tester->getWindowIcon(1000);
    m_tester->getWindowIcon(2000);
    m_tester->getWindowIcon(3000);
}

TEST_F(UT_WMWindowList, test_getWindowPid_multiple)
{
    // 测试获取多个窗口PID
    m_tester->getWindowPid(100);
    m_tester->getWindowPid(200);
    m_tester->getWindowPid(300);
}

TEST_F(UT_WMWindowList, test_isDesktopEntryApp_cacheManagement)
{
    // 测试桌面入口缓存管理
    pid_t pid1 = 3001;
    pid_t pid2 = 3002;
    pid_t pid3 = 3003;

    // 初始应该不存在
    EXPECT_FALSE(m_tester->isDesktopEntryApp(pid1));

    // 添加到缓存
    m_tester->m_desktopEntryCache << pid1;
    EXPECT_TRUE(m_tester->isDesktopEntryApp(pid1));

    // 添加更多
    m_tester->m_desktopEntryCache << pid2;
    m_tester->m_desktopEntryCache << pid3;
    EXPECT_TRUE(m_tester->isDesktopEntryApp(pid2));
    EXPECT_TRUE(m_tester->isDesktopEntryApp(pid3));
}

TEST_F(UT_WMWindowList, test_isGuiApp_cacheManagement)
{
    // 测试GUI应用缓存管理
    pid_t pid = 4001;
    EXPECT_FALSE(m_tester->isGuiApp(pid));

    // 添加到缓存
    m_tester->m_guiAppcache[pid] = std::make_unique<wm_window_t>();
    EXPECT_TRUE(m_tester->isGuiApp(pid));

    // 清理后应该不存在
    m_tester->m_guiAppcache.clear();
    EXPECT_FALSE(m_tester->isGuiApp(pid));
}

TEST_F(UT_WMWindowList, test_isTrayApp_cacheManagement)
{
    // 测试托盘应用缓存管理
    pid_t pid = 5001;
    EXPECT_FALSE(m_tester->isTrayApp(pid));

    // 添加到缓存
    m_tester->m_trayAppcache[pid] = std::make_unique<wm_window_t>();
    EXPECT_TRUE(m_tester->isTrayApp(pid));

    // 清理后应该不存在
    m_tester->m_trayAppcache.clear();
    EXPECT_FALSE(m_tester->isTrayApp(pid));
}

TEST_F(UT_WMWindowList, test_getWindowIcon_invalidPid)
{
    // 测试无效PID获取窗口图标
    m_tester->getWindowIcon(0);
    m_tester->getWindowIcon(-1);
}

TEST_F(UT_WMWindowList, test_getWindowPid_invalidWindow)
{
    // 测试无效窗口获取PID
    m_tester->getWindowPid(0);
    m_tester->getWindowPid(-1);
}
