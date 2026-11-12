// Copyright (C) 2019 ~ 2021 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

//self
#include "system/netif_monitor_thread.h"
#include "system/netif_monitor.h"
#include "system/netif_packet_capture.h"
#include "common/base_thread.h"

//gtest
#include "stub.h"
#include <gtest/gtest.h>

//qt
#include <QDebug>

//std
#include <atomic>
#include <chrono>
#include <functional>
#include <thread>

using namespace core::system;

// 用例计数声明（min/actual 对账，test-code-gen.md §4）:
// | 方法 | 级别 | 因子 | min | actual |
// |-----|------|------|-----|--------|
// |NetifMonitorThread()|mid|构造拉起内部线程链|1|1|
// |threadKey|low|常量线程键|1|1|
// |netifJobInstance|low|成员指针|1|1|
// |run|mid|requestQuit 后退出消费循环|1|1|
// 说明: dtor（B1）在每个用例 TearDown 的 delete m_tester 中执行，
// 不单独设立用例。本 TU 共 4 个用例。
//
// ============================ BRANCH MAP ============================
// 来源: get_code_snippet model::NetifMonitorThread 全部方法
//       @ system/netif_monitor_thread.cpp:15-60
// ctor : 创建 NetifMonitor、moveToThread、connect(started->startNetmonitorJob)、
//        m_netIfmoniterThread.start()
// B1   : dtor -> requestQuit + 内部线程 quit/wait(200)/terminate 兜底 + 自身退出
// B2   : netifJobInstance -> m_netifMonitor
// B3   : run -> m_netifMonitor->handleNetData()（quit 置位后循环退出返回）
// startNetmonitorJob 槽经 NetifPacketCapture::startNetifMonitorJob 打桩，
// 测试不触碰真实 pcap 设备。
// ====================================================================

/***************************************STUB begin*********************************************/

// 抓包任务桩：NetifMonitorThread 构造会拉起监控线程链，打桩避免真实 pcap 设备依赖
static std::atomic<int> g_captureJobCount {0};

static void stub_startNetifMonitorJob(void *obj)
{
    Q_UNUSED(obj);
    g_captureJobCount.fetch_add(1);
}

/***************************************STUB end**********************************************/

static bool utWaitFor(const std::function<bool()> &cond, int timeoutMs)
{
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    while (std::chrono::steady_clock::now() < deadline) {
        if (cond()) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return cond();
}

class UT_NetifMonitorThread: public ::testing::Test
{
public:
    UT_NetifMonitorThread() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        // 构造函数内部会启动线程并触发抓包任务，桩的生命周期需覆盖整个用例
        m_stub.set(ADDR(NetifPacketCapture, startNetifMonitorJob), stub_startNetifMonitorJob);
        g_captureJobCount.store(0);
        m_tester = new NetifMonitorThread();
    }

    virtual void TearDown()
    {
        if (m_tester) {
            // 先停外部线程（触发链源头），再停 NetifMonitor 内部抓包线程，最后析构
            if (m_tester->m_netIfmoniterThread.isRunning()) {
                m_tester->m_netIfmoniterThread.quit();
                m_tester->m_netIfmoniterThread.wait(3000);
            }
            if (m_tester->netifJobInstance()
                    && m_tester->netifJobInstance()->m_packetMonitorThread.isRunning()) {
                m_tester->netifJobInstance()->m_packetMonitorThread.quit();
                m_tester->netifJobInstance()->m_packetMonitorThread.wait(3000);
            }
            delete m_tester;
            m_tester = nullptr;
        }
    }

protected:
    Stub m_stub;
    NetifMonitorThread *m_tester;
};

// 构造：线程对象创建时同时创建网络监控任务
TEST_F(UT_NetifMonitorThread, NetifMonitorThread_Construction_NetifJobCreated)
{
    // Arrange: SetUp 已构造线程对象，先断言宿主可用
    ASSERT_NE(m_tester, nullptr);

    // Act
    NetifMonitor *job = m_tester->netifJobInstance();

    // Assert
    EXPECT_NE(job, nullptr);
    EXPECT_EQ(m_tester->netifJobInstance(), job);
}

// 线程标识：返回 kNetifMonitorThread 线程键
TEST_F(UT_NetifMonitorThread, ThreadKey_Returned_MatchesNetifThreadConstant)
{
    // Arrange: SetUp 已构造线程对象
    ASSERT_NE(m_tester, nullptr);

    // Act
    int key = m_tester->threadKey();

    // Assert: 线程键恒等于 kNetifMonitorThread
    EXPECT_EQ(key, static_cast<int>(BaseThread::kNetifMonitorThread));
    EXPECT_EQ(key, m_tester->threadKey());
}

// 构造副作用：内部线程启动后抓包任务槽被调用（已打桩）
TEST_F(UT_NetifMonitorThread, NetifMonitorThread_ThreadStarted_CaptureJobInvoked)
{
    // Arrange: SetUp 已把抓包任务槽打桩并计数清零
    ASSERT_NE(m_tester, nullptr);
    ASSERT_EQ(0, g_captureJobCount.load());

    // Act
    bool jobRan = utWaitFor([] { return g_captureJobCount.load() >= 1; }, 3000);

    // Assert: 槽被调用且内部事件线程存活
    EXPECT_EQ(1, g_captureJobCount.load());
    EXPECT_TRUE(jobRan);
}

// run 入口：requestQuit 置位后消费循环退出，run 正常返回
TEST_F(UT_NetifMonitorThread, Run_QuitRequested_ReturnsFromLoop)
{
    // Arrange
    NetifMonitor *job = m_tester->netifJobInstance();
    ASSERT_NE(job, nullptr);
    std::atomic<bool> finished {false};
    std::thread worker([&] {
        m_tester->run();
        finished.store(true);
    });

    // Act
    // 反复 requestQuit 直到 run 内的 handleNetData 退出（覆盖唤醒时序竞争）
    bool exited = utWaitFor([&] {
        job->requestQuit();
        return finished.load();
    }, 3000);
    worker.join();

    // Assert: run 正常返回且 quit 标志保持置位
    EXPECT_EQ(1, int(finished.load()));
    EXPECT_EQ(true, exited);
    EXPECT_EQ(1, int(job->m_quitRequested.load()));
}
