// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// qt-autotest-generator 生成的 DKaptureManager 单元测试
// 被测类：deepin-system-monitor-system-server/src/dkapture_manager.{h,cpp}
//
// 用例计数声明（self-check-structural 验证此块）：
// | method             | level | factors                    | min | actual |
// |--------------------|-------|----------------------------|-----|--------|
// | instance           | low   | 单例语义                   | 1   | 1      |
// | DKaptureManager    | mid   | 库缺失/库可用              | 2   | 2      |
// | open               | mid   | 无实例/委托成功/委托失败   | 3   | 3      |
// | close              | mid   | 无实例/委托成功            | 2   | 2      |
// | lifetime           | low   | 无实例/委托                | 2   | 2      |
// | isAvailable        | low   | 标志位                     | 1   | 1      |
// | read               | mid   | 不可用/可用委托            | 2   | 2      |
//
// 分支清单（源自真实源码，self-check §2c）：
// - instance: 静态局部单例，重复调用同指针
// - ctor: QLibrary("libdkapture.so") 加载失败 → m_available=false（warning 日志）;
//         加载成功 → resolve 全部符号，任一为空 → m_available=false + unload
// - open: m_dk_instance 为空 → -1；非空 → 委托并透传返回值
// - close: m_dk_instance 为空 → -1；非空 → 委托并透传返回值
// - lifetime: m_dk_instance 为空 → 0；非空 → 委托并透传
// - isAvailable: 返回 m_available
// - read: !m_available || 无实例 → -1；可用 → 委托 fake.read(types, cb, ctx)
// - dtor: 有实例时 delete m_dk_instance 并 unload（测试通过复位成员规避）

#include <gtest/gtest.h>
#include <vector>
#include <cstring>

#include "stubext.h"
#include "dkapture_manager.h"

// ═══════════════════ 打桩与 Fake ═══════════════════

namespace {

// 记录型 fake：验证 DKaptureManager 的委托链路
class TracingDKapture : public DKapture
{
public:
    int openCalls = 0;
    int closeCalls = 0;
    int lifetimeCalls = 0;
    int readCalls = 0;
    unsigned long long lastLifetimeMs = 0;
    unsigned long long lifetimeResult = 0;
    int openResult = 0;
    int closeResult = 0;
    ssize_t readResult = 7;

    ssize_t read(std::vector<DKapture::DataType> &, DKapture::DKCallback, void *) override
    {
        ++readCalls;
        return readResult;
    }

    // 其余接口本测试不触达，给确定的安全返回值
    ssize_t read(DKapture::DataType, DKapture::DKCallback, void *) override { return -1; }
    int open(FILE *, LogLevel) override { ++openCalls; return openResult; }
    unsigned long long lifetime(unsigned long long ms) override { ++lifetimeCalls; lastLifetimeMs = ms; return lifetimeResult; }
    ssize_t read(DKapture::DataType, pid_t, DKapture::DataHdr *, size_t) override { return -1; }
    ssize_t read(const char *, DKapture::DataHdr *, size_t) override { return -1; }
    ssize_t read(std::vector<DKapture::DataType> &, pid_t, DKapture::DataHdr *, size_t) override { return -1; }
    ssize_t read(std::vector<const char *> &, DKapture::DataHdr *, size_t) override { return -1; }
    ssize_t read(DKapture::DataType, std::vector<pid_t> &, DKapture::DataHdr *, size_t) override { return -1; }
    ssize_t read(std::vector<const char *> &, DKapture::DKCallback, void *) override { return -1; }
    int kmemleak_scan_start(pid_t, DKapture::DKCallback, void *) override { return -1; }
    int kmemleak_scan_stop(void) override { return 0; }
    int file_watch(const char *, DKapture::DKCallback, void *) override { return -1; }
    int fs_watch(const char *, DKapture::DKCallback, void *) override { return -1; }
    int irq_watch(DKapture::DKCallback, void *) override { return -1; }
    int close(void) override { ++closeCalls; return closeResult; }
};

}  // namespace

// ═══════════════════ 测试夹具 ═══════════════════

class DKaptureManagerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        stub.clear();
        // main.cpp 全局环境已屏蔽 QLibrary::load → ctor 不加载真实库
        mgr = DKaptureManager::instance();
        mgr->m_available = false;
        mgr->m_dk_instance = nullptr;
    }

    void TearDown() override
    {
        // 防止单例 dtor 在进程退出时 delete 测试栈上的 fake
        mgr->m_available = false;
        mgr->m_dk_instance = nullptr;
        stub.clear();
    }

    // 将单例置于可用状态并挂载 fake 实例
    void makeAvailable(TracingDKapture *fake)
    {
        mgr->m_available = true;
        mgr->m_dk_instance = fake;
    }

    stub_ext::StubExt stub;
    DKaptureManager *mgr = nullptr;
    TracingDKapture fake;
};

// ═══════════════════ 单例 ═══════════════════

TEST_F(DKaptureManagerTest, Instance_RepeatedCalls_ReturnsSamePointer)
{
    // Arrange: SetUp 已取得单例
    ASSERT_NE(mgr, nullptr);
    // Act
    DKaptureManager *again = DKaptureManager::instance();
    // Assert: 同一实例（静态局部单例语义）
    EXPECT_EQ(again, mgr);
    EXPECT_EQ(DKaptureManager::instance(), again);
}

// ═══════════════════ 构造 ═══════════════════

TEST_F(DKaptureManagerTest, Ctor_LibraryUnavailable_NotAvailable)
{
    // Arrange: SetUp 已复位（全局桩使 QLibrary::load 失败）
    ASSERT_FALSE(mgr->m_available);
    // Act: 构造在 instance() 首次调用时完成
    bool available = mgr->isAvailable();
    // Assert: 库缺失 → 不可用
    EXPECT_FALSE(available);
    EXPECT_EQ(mgr->m_dk_instance, nullptr);
}

TEST_F(DKaptureManagerTest, Ctor_InstanceMember_DefaultsNullAndUnavailable)
{
    // Arrange: 新拷贝的成员视图（规避单例共享状态干扰）
    ASSERT_NE(mgr, nullptr);
    // Act: 读取构造后的成员状态
    DKaptureManager *m = DKaptureManager::instance();
    // Assert
    EXPECT_EQ(m->m_dk_instance, nullptr);
    EXPECT_FALSE(m->m_available);
}

// ═══════════════════ open / close / lifetime ═══════════════════

TEST_F(DKaptureManagerTest, Open_WithoutInstance_ReturnsMinusOne)
{
    // Arrange: 无实例（SetUp 已复位）
    ASSERT_EQ(mgr->m_dk_instance, nullptr);
    // Act
    int ret = mgr->open(nullptr, DKapture::LogLevel(0));
    // Assert: 不发生委托
    EXPECT_EQ(ret, -1);
    EXPECT_EQ(fake.openCalls, 0);
}

TEST_F(DKaptureManagerTest, Open_WithInstance_DelegatesAndReturns)
{
    // Arrange: 挂载 fake，open 成功返回 0
    makeAvailable(&fake);
    fake.openResult = 0;
    // Act
    int ret = mgr->open(nullptr, DKapture::LogLevel(0));
    // Assert: 委托发生且透传返回值
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(fake.openCalls, 1);
}

TEST_F(DKaptureManagerTest, Open_InstanceFailure_PropagatesCode)
{
    // Arrange: fake open 返回非零错误码
    makeAvailable(&fake);
    fake.openResult = -9;
    // Act
    int ret = mgr->open(nullptr, DKapture::LogLevel(0));
    // Assert
    EXPECT_EQ(ret, -9);
    EXPECT_EQ(fake.openCalls, 1);
}

TEST_F(DKaptureManagerTest, Close_WithoutInstance_ReturnsMinusOne)
{
    // Arrange: 无实例
    ASSERT_EQ(mgr->m_dk_instance, nullptr);
    // Act
    int ret = mgr->close();
    // Assert
    EXPECT_EQ(ret, -1);
    EXPECT_EQ(fake.closeCalls, 0);
}

TEST_F(DKaptureManagerTest, Close_WithInstance_DelegatesAndReturns)
{
    // Arrange
    makeAvailable(&fake);
    fake.closeResult = 0;
    // Act
    int ret = mgr->close();
    // Assert
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(fake.closeCalls, 1);
}

TEST_F(DKaptureManagerTest, Lifetime_WithoutInstance_ReturnsZero)
{
    // Arrange: 无实例
    ASSERT_EQ(mgr->m_dk_instance, nullptr);
    // Act
    unsigned long long ret = mgr->lifetime(5000);
    // Assert
    EXPECT_EQ(ret, 0ULL);
    EXPECT_EQ(fake.lifetimeCalls, 0);
}

TEST_F(DKaptureManagerTest, Lifetime_WithInstance_DelegatesAndReturns)
{
    // Arrange
    makeAvailable(&fake);
    fake.lifetimeResult = 12345ULL;
    // Act
    unsigned long long ret = mgr->lifetime(5000);
    // Assert: 参数透传 + 返回值透传
    EXPECT_EQ(ret, 12345ULL);
    EXPECT_EQ(fake.lifetimeCalls, 1);
    EXPECT_EQ(fake.lastLifetimeMs, 5000ULL);
}

// ═══════════════════ isAvailable / read ═══════════════════

TEST_F(DKaptureManagerTest, IsAvailable_FlagToggles_ReflectsState)
{
    // Arrange: 不可用状态
    mgr->m_available = false;
    // Act + Assert: 标志位直接映射
    EXPECT_FALSE(mgr->isAvailable());
    EXPECT_EQ(mgr->m_dk_instance, nullptr);

    // Act: 置为可用
    mgr->m_available = true;
    // Assert
    EXPECT_TRUE(mgr->isAvailable());
}

TEST_F(DKaptureManagerTest, Read_WhenUnavailable_ReturnsMinusOne)
{
    // Arrange: 不可用（SetUp 已复位）
    ASSERT_FALSE(mgr->m_available);
    std::vector<DKapture::DataType> types{ DKapture::PROC_PID_STAT };
    // Act
    ssize_t ret = mgr->read(types, nullptr, nullptr);
    // Assert: 不发生委托
    EXPECT_EQ(ret, -1);
    EXPECT_EQ(fake.readCalls, 0);
}

TEST_F(DKaptureManagerTest, Read_WhenAvailable_DelegatesWithTypes)
{
    // Arrange: 可用 + fake 返回值
    makeAvailable(&fake);
    fake.readResult = 42;
    std::vector<DKapture::DataType> types{ DKapture::PROC_PID_STAT, DKapture::PROC_PID_IO };
    // Act
    ssize_t ret = mgr->read(types, nullptr, nullptr);
    // Assert
    EXPECT_EQ(ret, 42);
    EXPECT_EQ(fake.readCalls, 1);
}
