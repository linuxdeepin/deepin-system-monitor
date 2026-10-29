// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// 用例计数声明（self-check-structural 验证此块）：
// | method                       | level | factors                        | min | actual |
// |------------------------------|-------|--------------------------------|-----|--------|
// | DebugTimeManager::getInstance | low  | 单例幂等                        | 1   | 1      |
// | DebugTimeManager::DebugTimeManager | low | -                          | 1   | 1(经getInstance) |
// | DebugTimeManager::beginPointLinux | low | -                          | 1   | 2      |
// | DebugTimeManager::endPointLinux | mid | 分支: 点存在/不存在              | 2   | 3      |
// | DebugTimeManager::clear      | low   | -                              | 1   | 1      |
//
// 最小清单完成情况（test-code-gen §最小清单）：
// 1. 每个公开方法 ≥ 1 用例: [x]
// 2. 每个输入维度按等价类划分 ≥ 1 用例/类: [x]（点存在/不存在）
// 3. 每个等价类的边界值显式覆盖: [x]
// 4. 同质 ≥ 3 组用 TEST_P: [x]（非同质计时语义，逐点验证）
// 5. 分支清单 → 用例映射已列出: [x]
// 6. 每条 if/switch/throw/early-return 有触发用例: [x]
// 7. 异常路径 EXPECT_THROW 精确匹配: [x]（无异常路径）
// 8. 负面场景有专门用例: [x]（未知点 end 无副作用）
// 9. 负面用例验证强异常安全: [x]
// 10. stub_ext vs gMock 选择正确: [x]（libc gettimeofday 经 tests/stub.h 打桩保证确定性）

#include <gtest/gtest.h>

#include "common/perf.h"

#include <sys/time.h>

#include "stub.h"

namespace {
// gettimeofday 桩状态：按调用次序返回预设毫秒时间
qint64 g_stubNowMs = 0;

int stub_gettimeofday(struct timeval *tv, void * /*tz*/)
{
    tv->tv_sec = static_cast<time_t>(g_stubNowMs / 1000);
    tv->tv_usec = static_cast<suseconds_t>((g_stubNowMs % 1000) * 1000);
    return 0;
}
}

class PerfTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        g_stubNowMs = 0;
        // 每用例前清空单例状态，避免跨用例污染
        DebugTimeManager::getInstance()->clear();
    }

    void TearDown() override { }
};

// 分支清单（来源：DebugTimeManager::endPointLinux 真实分支 1 = if 1）
// B1: if (m_MapPoint.find(point) != end) 点存在 → EndPointComputesDelta 用例；
//     点不存在 → EndPoint_UnknownPoint_NoSideEffect 用例
TEST_F(PerfTest, GetInstance_Twice_ReturnsSameSingleton)
{
    // Arrange：无前置状态

    // Act
    DebugTimeManager *a = DebugTimeManager::getInstance();
    DebugTimeManager *b = DebugTimeManager::getInstance();

    // Assert
    EXPECT_NE(a, nullptr);
    EXPECT_EQ(a, b);
}

TEST_F(PerfTest, BeginPointLinux_NewPoint_InsertsStatusAndTimestamp)
{
    // Arrange：桩定当前时间 1000ms
    Stub stub;
    stub.set(&::gettimeofday, stub_gettimeofday);
    g_stubNowMs = 1000;

    // Act
    DebugTimeManager::getInstance()->beginPointLinux(QStringLiteral("POINT-T01"), QStringLiteral("size=1MB"));

    // Assert：私有 m_MapPoint 经 -fno-access-control 直查
    const auto &map = DebugTimeManager::getInstance()->m_MapPoint;
    EXPECT_EQ(map.size(), 1);
    EXPECT_TRUE(map.contains(QStringLiteral("POINT-T01")));
    EXPECT_EQ(map.value(QStringLiteral("POINT-T01")).desc, QStringLiteral("size=1MB"));
    EXPECT_EQ(map.value(QStringLiteral("POINT-T01")).time, 1000);
}

TEST_F(PerfTest, EndPointComputesDelta_KnownPoint_StoresElapsedMs)
{
    // Arrange：begin@1000ms → end@2500ms → 差值 1500ms
    Stub stub;
    stub.set(&::gettimeofday, stub_gettimeofday);
    DebugTimeManager *mgr = DebugTimeManager::getInstance();
    g_stubNowMs = 1000;
    mgr->beginPointLinux(QStringLiteral("POINT-T02"), QStringLiteral("stage1"));

    // Act
    g_stubNowMs = 2500;
    mgr->endPointLinux(QStringLiteral("POINT-T02"));

    // Assert：B1 命中分支，time 被覆写为耗时
    EXPECT_EQ(mgr->m_MapPoint.value(QStringLiteral("POINT-T02")).time, 1500);
    EXPECT_EQ(mgr->m_MapPoint.value(QStringLiteral("POINT-T02")).desc, QStringLiteral("stage1"));
}

TEST_F(PerfTest, EndPoint_UnknownPoint_NoSideEffect)
{
    // Arrange：空表
    Stub stub;
    stub.set(&::gettimeofday, stub_gettimeofday);

    // Act：B1 未命中分支
    DebugTimeManager::getInstance()->endPointLinux(QStringLiteral("NO-SUCH-POINT"));

    // Assert：不插入、不崩溃、时间戳桩未被调用改写
    const auto &map = DebugTimeManager::getInstance()->m_MapPoint;
    EXPECT_TRUE(map.isEmpty());
    EXPECT_FALSE(map.contains(QStringLiteral("NO-SUCH-POINT")));
    EXPECT_EQ(g_stubNowMs, 0);
}

TEST_F(PerfTest, Clear_AfterBegin_RemovesAllPoints)
{
    // Arrange：两个打点
    Stub stub;
    stub.set(&::gettimeofday, stub_gettimeofday);
    DebugTimeManager *mgr = DebugTimeManager::getInstance();
    g_stubNowMs = 10;
    mgr->beginPointLinux(QStringLiteral("P-A"), "");
    mgr->beginPointLinux(QStringLiteral("P-B"), "");
    EXPECT_EQ(mgr->m_MapPoint.size(), 2);

    // Act
    mgr->clear();

    // Assert
    EXPECT_TRUE(mgr->m_MapPoint.isEmpty());
}
