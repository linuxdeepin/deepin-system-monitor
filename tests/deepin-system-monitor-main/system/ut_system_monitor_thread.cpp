// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Unit tests for core::system::SystemMonitorThread
// (deepin-system-monitor-main/system/system_monitor_thread.cpp).
// Worker-thread lifecycle tests; periodic data sources are stubbed so the
// worker only runs cheap update cycles.

#include <gtest/gtest.h>

#include <QPointer>
#include <QSignalSpy>
#include <QTest>

#include "3rdparty/stub-ext/stubext.h"
#include "common/base_thread.h"
#include "process/process_db.h"
#include "system/device_db.h"
#include "system/sys_info.h"
#include "system/system_monitor.h"
#include "system/system_monitor_thread.h"

using stub_ext::StubExt;

namespace {

class UT_SystemMonitorThread : public ::testing::Test
{
protected:
    void SetUp() override
    {
        installUpdateStubs();
    }

    void TearDown() override
    {
    }

    void installUpdateStubs()
    {
        m_stub.set_lamda(&core::system::SysInfo::readSysInfo,
                         [](core::system::SysInfo *) -> void { });
        m_stub.set_lamda(&core::system::DeviceDB::update,
                         [](core::system::DeviceDB *) -> void { });
        m_stub.set_lamda(&core::process::ProcessDB::update,
                         [](core::process::ProcessDB *) -> void { });
    }

    StubExt m_stub;
};

// ---------------------------------------------------------------------------
// Construction owns exactly one SystemMonitor moved onto this thread and does
// not start anything until QThread::start.
// Branch: ctor -> new SystemMonitor + moveToThread(this) + started->startMonitorJob.
// ---------------------------------------------------------------------------
TEST_F(UT_SystemMonitorThread, Construction_OwnsMonitor_NotYetRunning)
{
    // Act
    core::system::SystemMonitorThread thread;
    QSignalSpy statSpy(thread.systemMonitorInstance(),
                       &core::system::SystemMonitor::statInfoUpdated);

    // Assert
    EXPECT_EQ(common::core::BaseThread::kSystemMonitorThread, thread.threadKey());
    ASSERT_NE(nullptr, thread.systemMonitorInstance());
    EXPECT_EQ(&thread, thread.systemMonitorInstance()->thread());   // affinity moved
    EXPECT_FALSE(thread.isRunning());
    // queued startMonitorJob must not have run yet
    QTest::qWait(50);
    EXPECT_EQ(0, statSpy.count());
}

// ---------------------------------------------------------------------------
// threadKey() override identifies the system-monitor slot.
// Branch: threadKey -> BaseThread::kSystemMonitorThread.
// ---------------------------------------------------------------------------
TEST_F(UT_SystemMonitorThread, ThreadKey_MatchesSystemMonitorSlot)
{
    // Arrange
    core::system::SystemMonitorThread thread;

    // Act / Assert
    EXPECT_EQ(common::core::BaseThread::kSystemMonitorThread, thread.threadKey());
}

// ---------------------------------------------------------------------------
// Start runs the monitor job in the worker thread: one forced refresh plus a
// statInfoUpdated emission queued back to the main thread.
// Branch: started -> startMonitorJob -> updateSystemMonitorInfo -> emit.
// ---------------------------------------------------------------------------
TEST_F(UT_SystemMonitorThread, Start_TriggersMonitorJob_CrossThread)
{
    // Arrange
    core::system::SystemMonitorThread thread;
    QSignalSpy statSpy(thread.systemMonitorInstance(),
                       &core::system::SystemMonitor::statInfoUpdated);

    // Act
    thread.start(QThread::LowPriority);
    QTRY_COMPARE_WITH_TIMEOUT(statSpy.count(), 1, 3000);

    // Assert
    EXPECT_EQ(1, statSpy.count());
    thread.quit();
    thread.wait(500);
}

// ---------------------------------------------------------------------------
// Destructor of a never-started thread deletes its monitor.
// Branch: ~SystemMonitorThread with isRunning() false -> skip quit/terminate
// -> delete m_monitor.
// ---------------------------------------------------------------------------
TEST_F(UT_SystemMonitorThread, Dtor_NotStarted_DeletesMonitor)
{
    // Arrange
    QPointer<core::system::SystemMonitor> guard;

    // Act
    {
        core::system::SystemMonitorThread thread;
        guard = thread.systemMonitorInstance();
        ASSERT_FALSE(guard.isNull());
    }

    // Assert
    EXPECT_TRUE(guard.isNull());
}

// ---------------------------------------------------------------------------
// Destructor of a running thread quits it, waits, then deletes the monitor.
// Branch: ~SystemMonitorThread with isRunning() true -> quit + wait(200)
// (no terminate on normal exit) -> delete m_monitor.
// ---------------------------------------------------------------------------
TEST_F(UT_SystemMonitorThread, Dtor_Running_QuitsAndDeletesMonitor)
{
    // Arrange
    QPointer<core::system::SystemMonitor> guard;

    {
        core::system::SystemMonitorThread thread;
        guard = thread.systemMonitorInstance();
        QSignalSpy statSpy(guard, &core::system::SystemMonitor::statInfoUpdated);
        thread.start(QThread::LowPriority);
        QTRY_COMPARE_WITH_TIMEOUT(statSpy.count(), 1, 3000);
        ASSERT_TRUE(thread.isRunning());
    }   // dtor runs here: quit + wait(200) + delete m_monitor

    // Assert
    EXPECT_TRUE(guard.isNull());
}

} // namespace
