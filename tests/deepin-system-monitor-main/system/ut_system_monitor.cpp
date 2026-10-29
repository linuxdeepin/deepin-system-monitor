// Copyright (C) 2019 ~ 2021 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

//self
#include "system/system_monitor.h"
#include "process/process_set.h"
//gtest
#include "stub.h"
#include <gtest/gtest.h>

//qt
#include <QTimerEvent>

using namespace core::system;

class UT_SystemMonitor: public ::testing::Test
{
public:
    UT_SystemMonitor() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        m_tester = new SystemMonitor();
    }

    virtual void TearDown()
    {
        if (m_tester) {
            delete m_tester;
            m_tester = nullptr;
        }
    }

protected:
    SystemMonitor *m_tester;
};

TEST_F(UT_SystemMonitor, initTest)
{
}

TEST_F(UT_SystemMonitor, test_instance)
{
    EXPECT_TRUE(m_tester->instance() != nullptr);
}

TEST_F(UT_SystemMonitor, test_deviceDB)
{
    EXPECT_TRUE(m_tester->deviceDB() != nullptr);
}

TEST_F(UT_SystemMonitor, test_processDB)
{
    EXPECT_TRUE(m_tester->processDB() != nullptr);
}

TEST_F(UT_SystemMonitor, test_sysInfo)
{
    EXPECT_TRUE(m_tester->sysInfo() != nullptr);
}

TEST_F(UT_SystemMonitor, DISABLED_test_startMonitorJob)
{
    m_tester->startMonitorJob();
    EXPECT_TRUE(m_tester->m_basictimer.isActive() == true);
}

TEST_F(UT_SystemMonitor, test_timerEvent)
{
    QTimerEvent event(1);
    m_tester->timerEvent(&event);
}

// 扩展测试：覆盖更多分支
TEST_F(UT_SystemMonitor, test_timerEvent_multipleTimers)
{
    // 测试多个定时器事件
    QTimerEvent event1(1);
    QTimerEvent event2(2);
    QTimerEvent event3(3);
    m_tester->timerEvent(&event1);
    m_tester->timerEvent(&event2);
    m_tester->timerEvent(&event3);
}

TEST_F(UT_SystemMonitor, test_instance_const)
{
    // 测试 const 版本
    const SystemMonitor* constMonitor = m_tester->instance();
    EXPECT_TRUE(constMonitor != nullptr);
}

TEST_F(UT_SystemMonitor, test_instance_multipleCalls)
{
    // 测试 instance() 多次调用返回同一实例
    SystemMonitor* instance1 = m_tester->instance();
    SystemMonitor* instance2 = SystemMonitor::instance();
    EXPECT_EQ(instance1, instance2);
}

TEST_F(UT_SystemMonitor, test_deviceDB_type)
{
    // 测试 deviceDB() 返回类型
    auto* db = m_tester->deviceDB();
    EXPECT_NE(db, nullptr);
    (void)db;  // 消除未使用警告
}

TEST_F(UT_SystemMonitor, test_processDB_type)
{
    // 测试 processDB() 返回类型
    auto* db = m_tester->processDB();
    EXPECT_NE(db, nullptr);
    (void)db;
}

TEST_F(UT_SystemMonitor, test_sysInfo_type)
{
    // 测试 sysInfo() 返回类型
    auto* info = m_tester->sysInfo();
    EXPECT_NE(info, nullptr);
    (void)info;
}
