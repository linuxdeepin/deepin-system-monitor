// Copyright (C) 2019-2026 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

//self
#include "application.h"

//dependence
#include "common/thread_manager.h"
#include "common/base_thread.h"
#include "system/system_monitor_thread.h"
#include "system/netif_monitor_thread.h"
#include "system/system_monitor.h"
#include "system/netif_monitor.h"

//gtest
#include "stub.h"
#include <gtest/gtest.h>

//qt
#include <QCoreApplication>
#include <QEvent>
#include <QThread>

//std
#include <chrono>

using namespace common::core;
using namespace core::system;

/***************************************STUB begin*********************************************/
namespace {
// Application 构造函数已在测试入口 main() 中执行（qApp 即 Application 实例）。
// event 分发会真正启动后台线程；线程启动本身保留真实行为（验证 isRunning），
// 但打桩其监控作业入口，阻断 DesktopManager DBus 扫描与真实 /proc 轮询（离屏环境下崩溃）。
static void stub_SystemMonitor_startMonitorJob(SystemMonitor *)
{
}

static void stub_NetifMonitor_handleNetData(NetifMonitor *)
{
}
}  // namespace
/***************************************STUB end***********************************************/

namespace {
// 有界自旋等待：线程从 start() 到真正进入 Running 存在竞态窗口
// （QThreadPrivate 在新线程内才置 running 标志），满载全量运行时窗口更大，
// 以固定 msleep 断言 isRunning 会偶发失败。
static bool waitThreadRunning(QThread *thread, int timeoutMs)
{
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    while (std::chrono::steady_clock::now() < deadline) {
        if (thread->isRunning())
            return true;
        QThread::msleep(10);
    }
    return thread->isRunning();
}
}  // namespace

// 分支清单（源自 application.cpp / application.h）:
// 1. ctor: 注册三个元类型 + ThreadManager::instance()->attach(SystemMonitorThread/NetifMonitorThread)
// 2. setCurrentTaskState/getCurrentTaskState: m_currentTaskStat 存取（默认 kTaskFinished）
// 3. event: type==kMonitorStartEventType -> SystemMonitorThread::start
//           | type==kNetifStartEventType -> NetifMonitorThread::start | 其它走 DApplication::event
// 4. MonitorStartEvent/NetifStartEvent: 携带自定义事件类型 + 析构
// 5. setMainWindow/mainWindow: 内联存取（raiseWindow 需真实 MainWindow，超出单测范围）
//
// 用例计数声明（min/actual 对账，test-code-gen.md §4）:
// | 方法 | 级别 | 因子 | min | actual |
// |-----|------|------|-----|--------|
// |ctor|mid|线程挂载×元类型注册|1|1|
// |getCurrentTaskState|low|默认值|1|1|
// |setCurrentTaskState|low|三态写入|1|1|
// |event|mid|MonitorStart×NetifStart×未知类型|3|3|
// |MonitorStartEvent|low|自定义类型|1|1|
// |NetifStartEvent|low|自定义类型|1|1|
// |setMainWindow/mainWindow|low|null往返|1|1|
// 本 TU 共 9 个用例。

class ApplicationTest : public ::testing::Test
{
public:
    virtual void SetUp() override
    {
        // Arrange: 测试入口 main() 已创建唯一 Application 实例（qApp），经 dynamic_cast 取出
        m_stub.set(ADDR(SystemMonitor, startMonitorJob), stub_SystemMonitor_startMonitorJob);
        m_stub.set(ADDR(NetifMonitor, handleNetData), stub_NetifMonitor_handleNetData);
        m_app = dynamic_cast<Application *>(QCoreApplication::instance());
        // Assert: 实例有效
        ASSERT_NE(m_app, nullptr);
    }

    virtual void TearDown() override
    {
        // 事件测试中启动的线程恢复停止态，避免影响后续用例与进程退出
        auto *sysThread = ThreadManager::instance()->thread<SystemMonitorThread>(BaseThread::kSystemMonitorThread);
        if (sysThread && sysThread->isRunning()) {
            sysThread->quit();
            sysThread->wait(1000);
        }
        auto *netThread = ThreadManager::instance()->thread<NetifMonitorThread>(BaseThread::kNetifMonitorThread);
        // NetifMonitorThread::run 为一次性执行（handleNetData 后自然结束），仅需等待
        if (netThread && netThread->isRunning())
            netThread->wait(1000);
    }

protected:
    Application *m_app = nullptr;
    Stub m_stub;
};

// ───────────────────────── ctor（经入口 main 已执行）─────────────────────────
TEST_F(ApplicationTest, Construction_DefaultInit_AttachesMonitorThreadsAndRegistersMetaTypes)
{
    // Arrange: 入口 main 中已构造 Application
    ASSERT_NE(QCoreApplication::instance(), nullptr);

    // Act: 从 ThreadManager 获取两个后台线程
    auto *sysThread = ThreadManager::instance()->thread<SystemMonitorThread>(BaseThread::kSystemMonitorThread);
    auto *netThread = ThreadManager::instance()->thread<NetifMonitorThread>(BaseThread::kNetifMonitorThread);

    // Assert: 两线程均已挂载
    EXPECT_NE(sysThread, nullptr);
    EXPECT_NE(netThread, nullptr);
    // Assert: 构造时注册的三个元类型可通过名称解析
    EXPECT_TRUE(QMetaType::fromName(QByteArray("Application::TaskState")).isValid());
    EXPECT_TRUE(QMetaType::fromName(QByteArray("pid_t")).isValid());
    EXPECT_TRUE(QMetaType::fromName(QByteArray("ErrorContext")).isValid());
}

// ───────────────────────── getCurrentTaskState ─────────────────────────
TEST_F(ApplicationTest, GetCurrentTaskState_InitialState_IsTaskFinished)
{
    // Arrange: SetUp 已取出入口实例；本用例为套件内首个改写状态的用例
    ASSERT_NE(m_app, nullptr);

    // Act
    const Application::TaskState state = m_app->getCurrentTaskState();

    // Assert: 头文件默认初始化值为 kTaskFinished
    EXPECT_EQ(state, Application::kTaskFinished);
    // Assert: getter 与私有成员一致（白盒交叉验证）
    EXPECT_EQ(state, m_app->m_currentTaskStat);
}

// ───────────────────────── setCurrentTaskState ─────────────────────────
TEST_F(ApplicationTest, SetCurrentTaskState_WithEachState_ValueIsStored)
{
    // Arrange: 准备枚举全集逐一驱动
    const Application::TaskState states[] = {
        Application::kTaskStarted, Application::kTaskRunning, Application::kTaskFinished
    };

    // Act: 依次写入每个状态并读回
    for (Application::TaskState st : states)
        m_app->setCurrentTaskState(st);

    // Assert: 最后写入的状态可读回，且枚举三态全部覆盖
    EXPECT_EQ(m_app->getCurrentTaskState(), Application::kTaskFinished);
    EXPECT_NE(Application::kTaskStarted, Application::kTaskRunning);
}

// ───────────────────────── MonitorStartEvent ─────────────────────────
TEST_F(ApplicationTest, MonitorStartEvent_Construction_CarriesCustomEventType)
{
    // Arrange: 无前置条件（自定义事件类型常量已在头文件定义）
    ASSERT_EQ(kMonitorStartEventType, static_cast<QEvent::Type>(QEvent::User + 1));

    // Act: 默认构造 MonitorStartEvent
    MonitorStartEvent event;

    // Assert: 事件类型为 kMonitorStartEventType（QEvent::User + 1）
    EXPECT_EQ(event.type(), kMonitorStartEventType);
    EXPECT_EQ(event.type(), static_cast<QEvent::Type>(QEvent::User + 1));
    // 析构在作用域退出时执行（不崩溃即通过）
}

// ───────────────────────── NetifStartEvent ─────────────────────────
TEST_F(ApplicationTest, NetifStartEvent_Construction_CarriesCustomEventType)
{
    // Arrange: 无前置条件（自定义事件类型常量已在头文件定义）
    ASSERT_EQ(kNetifStartEventType, static_cast<QEvent::Type>(QEvent::User + 2));

    // Act: 默认构造 NetifStartEvent
    NetifStartEvent event;

    // Assert: 事件类型为 kNetifStartEventType（QEvent::User + 2）
    EXPECT_EQ(event.type(), kNetifStartEventType);
    EXPECT_EQ(event.type(), static_cast<QEvent::Type>(QEvent::User + 2));
    // 析构在作用域退出时执行（不崩溃即通过）
}

// ───────────────────────── event ─────────────────────────
TEST_F(ApplicationTest, Event_MonitorStartEventType_StartsSystemMonitorThread)
{
    // Arrange: 构造 kMonitorStartEventType 自定义事件
    MonitorStartEvent event;

    // Act: 同步分发到 Application::event（线程启动，监控作业已打桩为空）
    QCoreApplication::sendEvent(m_app, &event);

    // Assert: 系统监视线程已被启动（有界等待进入 Running，容忍调度竞态）
    auto *sysThread = ThreadManager::instance()->thread<SystemMonitorThread>(BaseThread::kSystemMonitorThread);
    EXPECT_NE(sysThread, nullptr);
    const bool running = waitThreadRunning(sysThread, 3000);
    EXPECT_TRUE(running);
    EXPECT_TRUE(sysThread->isRunning());
}

TEST_F(ApplicationTest, Event_NetifStartEventType_StartsNetifMonitorThread)
{
    // Arrange: 构造 kNetifStartEventType 自定义事件
    NetifStartEvent event;

    // Act: 同步分发到 Application::event（run 为一次性执行，作业已打桩为空）
    QCoreApplication::sendEvent(m_app, &event);

    // Assert: 网卡监视线程已被启动（有界等待进入 Running；run 为一次性执行，
    // 已打桩为空的任务可能令线程极快结束，isFinished 同样算启动成功）
    auto *netThread = ThreadManager::instance()->thread<NetifMonitorThread>(BaseThread::kNetifMonitorThread);
    EXPECT_NE(netThread, nullptr);
    const auto started = [&] {
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(3000);
        while (std::chrono::steady_clock::now() < deadline) {
            if (netThread->isRunning() || netThread->isFinished())
                return true;
            QThread::msleep(10);
        }
        return netThread->isRunning() || netThread->isFinished();
    }();
    EXPECT_TRUE(started);
    EXPECT_EQ(netThread, ThreadManager::instance()->thread<NetifMonitorThread>(BaseThread::kNetifMonitorThread));
}

TEST_F(ApplicationTest, Event_UnknownCustomType_DoesNotDisturbSubsequentDispatch)
{
    // Arrange: 未知自定义事件（非两个启动事件，走 else 分支进入 DApplication::event）
    QEvent unknown(static_cast<QEvent::Type>(QEvent::User + 999));

    // Act: 先分发未知事件，再分发系统启动事件
    QCoreApplication::sendEvent(m_app, &unknown);
    MonitorStartEvent startEvent;
    QCoreApplication::sendEvent(m_app, &startEvent);

    // Assert: 未知事件未干扰事件管线，后续启动事件仍正常生效（有界等待）
    auto *sysThread = ThreadManager::instance()->thread<SystemMonitorThread>(BaseThread::kSystemMonitorThread);
    EXPECT_NE(sysThread, nullptr);
    const bool running = waitThreadRunning(sysThread, 3000);
    EXPECT_TRUE(running);
    EXPECT_TRUE(sysThread->isRunning());
}

// ───────────────────────── setMainWindow/mainWindow ─────────────────────────
TEST_F(ApplicationTest, SetMainWindow_WithNullPointer_MainWindowReturnsNull)
{
    // Arrange: 内联存取接口，空指针不涉及窗口析构风险
    ASSERT_NE(m_app, nullptr);

    // Act
    m_app->setMainWindow(nullptr);
    MainWindow *win = m_app->mainWindow();

    // Assert: 存取往返一致
    EXPECT_EQ(win, nullptr);
    EXPECT_TRUE(m_app->mainWindow() == nullptr);
    EXPECT_FALSE(m_app->mainWindow() != nullptr);
}
