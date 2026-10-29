// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Unit tests for core::system::SystemMonitor (deepin-system-monitor-main/system/system_monitor.cpp).
// Periodic-refresh plumbing only: the per-cycle data sources
// (SysInfo::readSysInfo, DeviceDB::update, ProcessDB::update) are stubbed;
// real /proc, DBus property reads and hardware scans never happen.

#include <gtest/gtest.h>

#include <QCoreApplication>
#include <QSignalSpy>
#include <QTimerEvent>

#include "3rdparty/stub-ext/stubext.h"
#include "common/thread_manager.h"
#include "process/process_db.h"
#include "system/device_db.h"
#include "system/sys_info.h"
#include "system/system_monitor.h"
#include "system/system_monitor_thread.h"
#include "ut_stub_helpers.h"

using stub_ext::StubExt;
using ut_helpers::DBusState;

namespace {

class UT_SystemMonitor : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_readSysInfoCalls = 0;
        m_deviceUpdateCalls = 0;
        m_processUpdateCalls = 0;
        m_dbus = DBusState();
        ut_helpers::installDBusStubs(m_stub, m_dbus);
        installUpdateStubs();
    }

    void TearDown() override
    {
    }

    void installUpdateStubs()
    {
        auto *readCalls = &m_readSysInfoCalls;
        m_stub.set_lamda(&core::system::SysInfo::readSysInfo,
                         [readCalls](core::system::SysInfo *) -> void {
                             ++(*readCalls);
                         });
        auto *devCalls = &m_deviceUpdateCalls;
        m_stub.set_lamda(&core::system::DeviceDB::update,
                         [devCalls](core::system::DeviceDB *) -> void {
                             ++(*devCalls);
                         });
        auto *procCalls = &m_processUpdateCalls;
        m_stub.set_lamda(&core::process::ProcessDB::update,
                         [procCalls](core::process::ProcessDB *) -> void {
                             ++(*procCalls);
                         });
    }

    StubExt m_stub;
    DBusState m_dbus;
    int m_readSysInfoCalls = 0;
    int m_deviceUpdateCalls = 0;
    int m_processUpdateCalls = 0;
};

// ---------------------------------------------------------------------------
// Construction creates the three data sources and reads static sys info once
// (hostname flows through the DBus property stub).
// Branch: ctor -> SysInfo/DeviceDB/ProcessDB news + readSysInfoStatic().
// ---------------------------------------------------------------------------
TEST_F(UT_SystemMonitor, Construction_CreatesDataSources_WithStaticInfo)
{
    // Act
    core::system::SystemMonitor mon;

    // Assert
    ASSERT_NE(nullptr, mon.sysInfo());
    ASSERT_NE(nullptr, mon.deviceDB());
    ASSERT_NE(nullptr, mon.processDB());
    EXPECT_EQ(QByteArray("ut-test-host"), mon.sysInfo()->hostname());
    // ctor performs exactly one static read, no periodic update yet
    EXPECT_EQ(0, m_readSysInfoCalls);
    EXPECT_EQ(0, m_deviceUpdateCalls);
    EXPECT_EQ(0, m_processUpdateCalls);
}

// ---------------------------------------------------------------------------
// startMonitorJob starts the basic timer and performs one forced refresh:
// readSysInfo + both DB updates + statInfoUpdated + recount (0,0) on an
// untouched ProcessSet.
// Branches: startMonitorJob -> global_init, timer start(1000),
// updateSystemMonitorInfo.
// ---------------------------------------------------------------------------
TEST_F(UT_SystemMonitor, StartMonitorJob_ForcesOneUpdateAndEmitsCounts)
{
    // Arrange
    core::system::SystemMonitor mon;
    QSignalSpy statSpy(&mon, &core::system::SystemMonitor::statInfoUpdated);
    QSignalSpy countSpy(&mon, &core::system::SystemMonitor::appAndProcCountUpdate);
    ASSERT_FALSE(mon.m_basictimer.isActive());

    // Act
    mon.startMonitorJob();

    // Assert: one forced update cycle
    EXPECT_EQ(1, m_readSysInfoCalls);
    EXPECT_EQ(1, m_deviceUpdateCalls);
    EXPECT_EQ(1, m_processUpdateCalls);
    EXPECT_EQ(1, statSpy.count());
    EXPECT_EQ(1, countSpy.count());
    // fresh ProcessSet knows no processes (payload: appCount, procCount)
    ASSERT_EQ(2, countSpy.first().size());
    EXPECT_EQ(0, countSpy.first().at(0).toInt());
    EXPECT_EQ(0, countSpy.first().at(1).toInt());
    // periodic timer now active with 1s interval
    EXPECT_TRUE(mon.m_basictimer.isActive());

    mon.m_basictimer.stop();
}

// ---------------------------------------------------------------------------
// startMonitorJob is idempotent w.r.t. the timer: a second call restarts the
// timer and forces a second full refresh.
// Branch: m_basictimer.stop() before re-start.
// ---------------------------------------------------------------------------
TEST_F(UT_SystemMonitor, StartMonitorJob_SecondCall_RefreshesAgain)
{
    // Arrange
    core::system::SystemMonitor mon;
    QSignalSpy statSpy(&mon, &core::system::SystemMonitor::statInfoUpdated);

    // Act
    mon.startMonitorJob();
    mon.startMonitorJob();

    // Assert
    EXPECT_EQ(2, m_readSysInfoCalls);
    EXPECT_EQ(2, m_deviceUpdateCalls);
    EXPECT_EQ(2, m_processUpdateCalls);
    EXPECT_EQ(2, statSpy.count());
    EXPECT_TRUE(mon.m_basictimer.isActive());

    mon.m_basictimer.stop();
}

// ---------------------------------------------------------------------------
// timerEvent alternates between the data-refresh branch and the
// emit-and-recount branch; a foreign timer id is ignored.
//
// The refresh counter is a file-static in system_monitor.cpp, so the first
// event's branch parity is deliberately not assumed: over two consecutive
// events exactly one takes each branch, giving deterministic totals:
//   updates = 1 (startMonitorJob) + 1 = 2,   statInfoUpdated = 1 + 1 = 2
// ---------------------------------------------------------------------------
TEST_F(UT_SystemMonitor, TimerEvent_AlternatesBranches_ForeignIdIgnored)
{
    // Arrange
    core::system::SystemMonitor mon;
    QSignalSpy statSpy(&mon, &core::system::SystemMonitor::statInfoUpdated);
    QSignalSpy countSpy(&mon, &core::system::SystemMonitor::appAndProcCountUpdate);
    mon.startMonitorJob();
    ASSERT_EQ(1, m_readSysInfoCalls);
    ASSERT_EQ(1, statSpy.count());

    // Act: two consecutive own-timer events (branches must alternate)
    QTimerEvent own1(mon.m_basictimer.timerId());
    QCoreApplication::sendEvent(&mon, &own1);
    QTimerEvent own2(mon.m_basictimer.timerId());
    QCoreApplication::sendEvent(&mon, &own2);

    // Assert: one update-branch event + one emit-branch event
    EXPECT_EQ(2, m_readSysInfoCalls);
    EXPECT_EQ(2, m_deviceUpdateCalls);
    EXPECT_EQ(2, m_processUpdateCalls);
    EXPECT_EQ(2, statSpy.count());
    EXPECT_EQ(2, countSpy.count());

    // Act: foreign timer id -> no state change
    const int readsBefore = m_readSysInfoCalls;
    const int statBefore = statSpy.count();
    QTimerEvent foreign(9999);
    QCoreApplication::sendEvent(&mon, &foreign);

    // Assert
    EXPECT_EQ(readsBefore, m_readSysInfoCalls);
    EXPECT_EQ(statBefore, statSpy.count());
    EXPECT_EQ(2, m_deviceUpdateCalls);

    mon.m_basictimer.stop();
}

// ---------------------------------------------------------------------------
// instance() resolves through ThreadManager to the attached thread's monitor.
// The harness may already have attached its own SystemMonitorThread, so the
// test reuses whatever thread currently holds the system-monitor slot.
// Branch: instance -> ThreadManager::thread<SystemMonitorThread>()
//          ->systemMonitorInstance().
// ---------------------------------------------------------------------------
TEST_F(UT_SystemMonitor, Instance_ReturnsThreadOwnedMonitor)
{
    // Arrange
    auto *tm = common::core::ThreadManager::instance();
    auto *attached = tm->thread<core::system::SystemMonitorThread>(
            common::core::BaseThread::kSystemMonitorThread);
    if (!attached) {
        attached = new core::system::SystemMonitorThread();
        tm->attach(attached);
    }
    ASSERT_NE(nullptr, attached);
    ASSERT_NE(nullptr, attached->systemMonitorInstance());

    // Act / Assert
    EXPECT_EQ(attached->systemMonitorInstance(), core::system::SystemMonitor::instance());
}

} // namespace
