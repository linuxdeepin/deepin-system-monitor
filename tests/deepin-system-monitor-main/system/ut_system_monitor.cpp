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
#include <QSignalSpy>
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

static DmiCpuInfo stub_readDmiCpuInfo_for_system_monitor()
{
    DmiCpuInfo info;
    info.hasCpuFrequency = true;
    info.cpuMHz = "1600";
    info.cpuMaxMHz = "3000";
    return info;
}

TEST_F(UT_SystemMonitor, test_startDmiCpuInfoLoadIfNeeded_01)
{
    m_tester->m_dmiCpuInfoReader = stub_readDmiCpuInfo_for_system_monitor;
    QSignalSpy spy(m_tester->m_dmiCpuInfoWatcher, &QFutureWatcher<DmiCpuInfo>::finished);

    m_tester->startDmiCpuInfoLoadIfNeeded();
    EXPECT_EQ(m_tester->m_dmiLoadState, SystemMonitor::DmiLoadState::Loading);

    ASSERT_TRUE(spy.wait(1000));
    EXPECT_EQ(m_tester->m_dmiLoadState, SystemMonitor::DmiLoadState::Loaded);

    m_tester->startDmiCpuInfoLoadIfNeeded();
    EXPECT_EQ(m_tester->m_dmiLoadState, SystemMonitor::DmiLoadState::Loaded);
}

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

TEST_F(UT_SystemMonitor, test_startMonitorJob)
{
    m_tester->m_dmiCpuInfoReader = stub_readDmiCpuInfo_for_system_monitor;
    QSignalSpy spy(m_tester->m_dmiCpuInfoWatcher, &QFutureWatcher<DmiCpuInfo>::finished);

    m_tester->startMonitorJob();
    EXPECT_TRUE(m_tester->m_basictimer.isActive() == true);
    ASSERT_TRUE(spy.wait(1000));
}

TEST_F(UT_SystemMonitor, test_timerEvent)
{
    QTimerEvent event(1);
    m_tester->timerEvent(&event);
}
