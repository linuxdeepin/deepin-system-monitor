// Copyright (C) 2019 ~ 2021 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

//self
#include "system/netif_monitor.h"
#include "system/netif_packet_capture.h"

//gtest
#include "stub.h"
#include <gtest/gtest.h>

//qt
#include <QDebug>
#include <QSharedPointer>

//std
#include <atomic>
#include <chrono>
#include <functional>
#include <thread>

using namespace core::system;

/***************************************STUB begin*********************************************/

// 抓包任务桩：替代 NetifPacketCapture::startNetifMonitorJob，避免真实 pcap 设备依赖，
// 仅计数验证"线程启动后任务槽被调用"
static std::atomic<int> g_captureJobCount {0};

static void stub_startNetifMonitorJob(void *obj)
{
    Q_UNUSED(obj);
    g_captureJobCount.fetch_add(1);
}

/***************************************STUB end**********************************************/

// 等待条件成立，deadline 内轮询，避免固定 sleep 造成的不稳定
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

static PacketPayload utMakePayload(ino_t ino, packet_direction dir, unsigned long long bytes)
{
    auto payload = QSharedPointer<struct packet_payload_t>::create();
    payload->ino = ino;
    payload->direction = dir;
    payload->payload = bytes;
    return payload;
}

class UT_NetifMonitor: public ::testing::Test
{
public:
    UT_NetifMonitor() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        m_tester = new NetifMonitor();
    }

    virtual void TearDown()
    {
        if (m_tester) {
            // 测试若启动过抓包线程，先停掉再析构，避免跨线程删除对象
            if (m_tester->m_packetMonitorThread.isRunning()) {
                m_tester->m_packetMonitorThread.quit();
                m_tester->m_packetMonitorThread.wait(3000);
            }
            delete m_tester;
            m_tester = nullptr;
        }
    }

protected:
    NetifMonitor *m_tester;
};

// 构造：抓包子任务随对象创建并挂接（ctor 将 this 传给 capture 的 netIfmontor 成员，
// 注意其 QObject parent 为空是源码现状）
TEST_F(UT_NetifMonitor, NetifMonitor_Construction_CaptureJobAttached)
{
    // Arrange
    // SetUp 中已构造 NetifMonitor

    // Act
    NetifPacketCapture *capture = m_tester->m_netifCapture;

    // Assert
    EXPECT_NE(capture, nullptr);
    EXPECT_EQ(capture->m_netifMonitor, m_tester);
}

// 单例入口：Application 启动时已向 ThreadManager 注册 NetifMonitorThread
TEST_F(UT_NetifMonitor, Instance_NetifThreadAttached_ReturnsNonNullJob)
{
    // Arrange
    // 测试进程 main.cpp 中 Application 构造已 attach NetifMonitorThread

    // Act
    NetifMonitor *job = m_tester->instance();

    // Assert
    EXPECT_NE(job, nullptr);
    EXPECT_EQ(m_tester->instance(), job);
}

// 启动监控：线程被拉起且抓包任务槽被调用（任务已打桩，不触真实设备）
TEST_F(UT_NetifMonitor, StartNetmonitorJob_ThreadStarted_CaptureJobInvoked)
{
    // Arrange
    Stub stub;
    stub.set(ADDR(NetifPacketCapture, startNetifMonitorJob), stub_startNetifMonitorJob);
    g_captureJobCount.store(0);

    // Act
    m_tester->startNetmonitorJob();
    bool jobRan = utWaitFor([] { return g_captureJobCount.load() >= 1; }, 3000);

    // Assert
    EXPECT_TRUE(jobRan);
    EXPECT_TRUE(m_tester->m_packetMonitorThread.isRunning());

    // 收尾：停止事件循环线程
    m_tester->m_packetMonitorThread.quit();
    EXPECT_TRUE(m_tester->m_packetMonitorThread.wait(3000));
}

// 消费循环：新 inode 入站包按载荷累加 rx 统计（覆盖"新建统计项"入站分支）
TEST_F(UT_NetifMonitor, HandleNetData_NewInodeInboundPackets_AccumulateRxStats)
{
    // Arrange
    m_tester->m_pendingPackets.enqueue(utMakePayload(100, kInboundPacket, 10));
    m_tester->m_pendingPackets.enqueue(utMakePayload(100, kInboundPacket, 20));
    m_tester->m_pendingPackets.enqueue(utMakePayload(300, kInboundPacket, 5));

    std::atomic<bool> finished {false};
    std::thread worker([&] {
        m_tester->handleNetData();
        finished.store(true);
    });

    // Act
    // 唤醒一次让 worker 消费 pending 队列，随后置退出标志并反复唤醒直到循环退出
    m_tester->m_pktqWatcher.wakeAll();
    bool consumed = utWaitFor([&] {
        m_tester->m_pktqWatcher.wakeAll();
        return m_tester->m_sockIOStatMap.contains(100) && m_tester->m_sockIOStatMap.contains(300);
    }, 3000);
    m_tester->requestQuit();
    bool exited = utWaitFor([&] {
        m_tester->m_pktqWatcher.wakeAll();
        return finished.load();
    }, 3000);
    worker.join();

    // Assert
    EXPECT_TRUE(consumed);
    EXPECT_TRUE(exited);
    ASSERT_TRUE(m_tester->m_sockIOStatMap.contains(100));
    auto stat100 = m_tester->m_sockIOStatMap[100];
    EXPECT_EQ(stat100->rx_bytes, 30ull);
    EXPECT_EQ(stat100->rx_packets, 2ull);
    EXPECT_EQ(stat100->tx_bytes, 0ull);
    ASSERT_TRUE(m_tester->m_sockIOStatMap.contains(300));
    auto stat300 = m_tester->m_sockIOStatMap[300];
    EXPECT_EQ(stat300->rx_bytes, 5ull);
    EXPECT_EQ(stat300->rx_packets, 1ull);
}

// 消费循环：既有 inode 出站包在原统计上累加 tx（覆盖"已有统计项"出站分支）
TEST_F(UT_NetifMonitor, HandleNetData_ExistingInodeOutboundPackets_AccumulateTxStats)
{
    // Arrange
    // 预置 inode 200 的既有统计，验证后续出站包在其基础上累加
    auto preset = QSharedPointer<struct sock_io_stat_t>::create();
    preset->ino = 200;
    preset->tx_bytes = 100;
    preset->tx_packets = 1;
    m_tester->m_sockIOStatMap.insert(200, preset);
    m_tester->m_pendingPackets.enqueue(utMakePayload(200, kOutboundPacket, 7));
    m_tester->m_pendingPackets.enqueue(utMakePayload(200, kOutboundPacket, 8));

    std::atomic<bool> finished {false};
    std::thread worker([&] {
        m_tester->handleNetData();
        finished.store(true);
    });

    // Act
    m_tester->m_pktqWatcher.wakeAll();
    bool consumed = utWaitFor([&] {
        m_tester->m_pktqWatcher.wakeAll();
        return m_tester->m_sockIOStatMap.contains(200)
               && m_tester->m_sockIOStatMap[200]->tx_bytes >= 115ull;
    }, 3000);
    m_tester->requestQuit();
    bool exited = utWaitFor([&] {
        m_tester->m_pktqWatcher.wakeAll();
        return finished.load();
    }, 3000);
    worker.join();

    // Assert
    EXPECT_TRUE(consumed);
    EXPECT_TRUE(exited);
    ASSERT_TRUE(m_tester->m_sockIOStatMap.contains(200));
    auto stat = m_tester->m_sockIOStatMap[200];
    EXPECT_EQ(stat->tx_bytes, 115ull);
    EXPECT_EQ(stat->tx_packets, 3ull);
    EXPECT_EQ(stat->rx_bytes, 0ull);
}

// 退出路径：requestQuit 置位后消费循环正常退出且不产生新统计
TEST_F(UT_NetifMonitor, HandleNetData_QuitRequested_ExitsLoopWithoutStats)
{
    // Arrange
    std::atomic<bool> finished {false};
    std::thread worker([&] {
        m_tester->handleNetData();
        finished.store(true);
    });

    // Act
    // 循环未投递任何包时直接置退出标志，worker 应在超时前退出
    bool exited = utWaitFor([&] {
        m_tester->requestQuit();
        return finished.load();
    }, 3000);
    worker.join();

    // Assert
    EXPECT_TRUE(finished.load());
    EXPECT_TRUE(exited);
    EXPECT_TRUE(m_tester->m_quitRequested.load());
    EXPECT_TRUE(m_tester->m_sockIOStatMap.isEmpty());
}

// 按 inode 取统计：缓存命中时返回统计并从缓存移除
TEST_F(UT_NetifMonitor, GetSockIOStatByInode_CachedInode_ReturnsStatAndEvicts)
{
    // Arrange
    auto preset = QSharedPointer<struct sock_io_stat_t>::create();
    preset->ino = 1001;
    preset->rx_bytes = 66;
    preset->rx_packets = 6;
    preset->tx_bytes = 55;
    preset->tx_packets = 5;
    m_tester->m_sockIOStatMap.insert(1001, preset);
    SockIOStat out;

    // Act
    bool ok = m_tester->getSockIOStatByInode(1001, out);

    // Assert
    EXPECT_TRUE(ok);
    EXPECT_EQ(out->ino, 1001u);
    EXPECT_EQ(out->rx_bytes, 66ull);
    EXPECT_EQ(out->rx_packets, 6ull);
    EXPECT_EQ(out->tx_bytes, 55ull);
    EXPECT_EQ(out->tx_packets, 5ull);
    EXPECT_FALSE(m_tester->m_sockIOStatMap.contains(1001));
}

// 按 inode 取统计：缓存未命中时返回 false
TEST_F(UT_NetifMonitor, GetSockIOStatByInode_UnknownInode_ReturnsFalse)
{
    // Arrange
    SockIOStat out;

    // Act
    bool ok = m_tester->getSockIOStatByInode(9999, out);

    // Assert
    EXPECT_FALSE(ok);
    EXPECT_TRUE(m_tester->m_sockIOStatMap.isEmpty());
}

// 请求退出：requestQuit 置位退出标志并唤醒等待中的消费循环
TEST_F(UT_NetifMonitor, RequestQuit_Called_SetsFlagAndWakesLoop)
{
    // Arrange
    std::atomic<bool> finished {false};
    std::thread worker([&] {
        m_tester->handleNetData();
        finished.store(true);
    });

    // Act
    // 等 worker 进入 wait 后调用 requestQuit，其内部 wakeAll 应使循环退出
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    m_tester->requestQuit();
    bool exited = utWaitFor([&] {
        m_tester->m_pktqWatcher.wakeAll();
        return finished.load();
    }, 3000);
    worker.join();

    // Assert
    EXPECT_TRUE(m_tester->m_quitRequested.load());
    EXPECT_TRUE(exited);
}
