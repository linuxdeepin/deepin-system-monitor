// Copyright (C) 2019 ~ 2021 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

//self
#include "system/netif_monitor.h"
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <atomic>
#include <thread>
#include <chrono>

//gtest
#include "stub.h"
#include <gtest/gtest.h>

//qt
#include <QDebug>
#include <QTimer>
#include <QWaitCondition>
#include <QMap>

using namespace core::system;

/***************************************STUB begin*********************************************/

bool stub_isEmpty_false()
{
    return false;
}

bool stub_wait(void* obj, QMutex *lockedMutex, unsigned long time = ULONG_MAX)
{
    return false;
}

template <class Key, class T>
bool stub_contains(void* obj, const int key)
{
    return false;
}

/***************************************STUB end**********************************************/


class UT_NetifMonitor: public QObject, public ::testing::Test
{
//    Q_OBJECT
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
            delete m_tester;
            m_tester = nullptr;
        }
    }

protected:
    NetifMonitor *m_tester;
};

TEST_F(UT_NetifMonitor, initTest)
{
}

TEST_F(UT_NetifMonitor, test_instace)
{
    EXPECT_TRUE(m_tester->instance() != nullptr);
}

TEST_F(UT_NetifMonitor, test_startNetmonitorJob)
{
    m_tester->startNetmonitorJob();
    EXPECT_TRUE( m_tester->m_packetMonitorThread.isRunning() == true);
}

TEST_F(UT_NetifMonitor, test_handleNetData)
{
    // handleNetData 是阻塞消费者循环：在 m_pktqWatcher.wait() 上阻塞。
    // stub.h 改写机器码钩函数，但对 Qt 库符号 QWaitCondition::wait 无效（已验证），
    // 故改用真实条件变量 + 线程驱动：在工作线程跑 handleNetData，主线程反复 wakeAll
    // 让其处理一拨 pending 后置 quit 再唤醒使其退出。反复 wakeAll 规避丢失唤醒。
    auto mkPayload = [](ino_t ino, packet_direction dir, unsigned long long bytes) {
        auto p = QSharedPointer<struct packet_payload_t>::create();
        p->ino = ino;
        p->direction = dir;
        p->payload = bytes;
        return p;
    };
    // 4 个载荷覆盖累加逻辑全部 4 分支：new/existing × inbound/outbound
    m_tester->m_pendingPackets.enqueue(mkPayload(100, kInboundPacket, 10));
    m_tester->m_pendingPackets.enqueue(mkPayload(100, kInboundPacket, 20));
    m_tester->m_pendingPackets.enqueue(mkPayload(200, kOutboundPacket, 5));
    m_tester->m_pendingPackets.enqueue(mkPayload(200, kOutboundPacket, 7));

    std::atomic<bool> finished {false};
    std::thread worker([&] {
        m_tester->handleNetData();
        finished.store(true);
    });

    // 让 worker 进入 wait，唤醒一次处理 pending（quit 仍为 false）
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    m_tester->m_pktqWatcher.wakeAll();
    // 待第一拨处理完成、循环回到 wait 后，置 quit 并反复唤醒确保退出唤醒不丢失
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    m_tester->m_quitRequested.store(true);
    for (int i = 0; i < 20; ++i) {
        m_tester->m_pktqWatcher.wakeAll();
        if (finished.load()) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    worker.join();

    // 断言按 inode 累加结果
    ASSERT_TRUE(m_tester->m_sockIOStatMap.contains(100));
    auto s100 = m_tester->m_sockIOStatMap[100];
    EXPECT_EQ(s100->rx_bytes, 30ull);   // 10 + 20
    EXPECT_EQ(s100->rx_packets, 2ull);
    EXPECT_EQ(s100->tx_bytes, 0ull);

    ASSERT_TRUE(m_tester->m_sockIOStatMap.contains(200));
    auto s200 = m_tester->m_sockIOStatMap[200];
    // 出站字节数为各包载荷直接累加：5(新建) + 7(已有) = 12。
    // 修正了生产代码 handleNetData 中累加分支 payload*2 的翻倍 bug
    // （与新建分支不一致、且无下游补偿，致按进程发送流量虚高）。
    EXPECT_EQ(s200->tx_bytes, 12ull);
    EXPECT_EQ(s200->tx_packets, 2ull);
    EXPECT_EQ(s200->rx_bytes, 0ull);
}

//TEST_F(UT_NetifMonitor, test_getSockIOStatByInode)
//{
//    Stub stub;
//    stub.set((bool(QMap::*)(int))ADDR(QMap,contains), stub_contains);
//    ino_t ino = 1;
//    SockIOStat stat;
//    qulonglong totalDelta = m_tester->getSockIOStatByInode(ino,stat);
//    EXPECT_NE(totalDelta, 0);
//}
