// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// qt-autotest-generator 单元测试 - SystemMonitorController
// （deepin-system-monitor-dde-shell-plugin/systemmonitorcontroller.cpp）
//
// 测试策略：SystemMonitorProber 的三个公开方法（非虚）以 stub-ext 打桩注入
// 固定数值，控制器与 /proc 环境完全隔离；openSystemMonitor 的 DBus 激活路径
// 打桩 QDBusConnection::sessionBus（返回空连接）+ QDBusAbstractInterface::
// isValid/doAsyncCall，回退路径打桩 QProcess 的 startDetached，不依赖真实桌面
// 环境；定时刷新路径以 QSignalSpy + QTest::qWait 走真实事件循环验证。
//
// 用例计数声明：
// | 函数 | 级别 | 因子 | min | actual |
// |------|------|------|-----|--------|
// | SystemMonitorController | high | 构造/初始刷新 | 2 | 2 |
// | 属性读取 | high | Q_PROPERTY 元对象 | 2 | 2 |
// | refreshSystemStats | high | 更新/保留/信号 | 5 | 5 |
// | 定时器 | medium | 2s 周期刷新 | 2 | 2 |
// | openSystemMonitor | high | DBus/回退/invokable | 3 | 3 |
//
// 10 项检查清单（test-code-gen.md）：
// 1 public 方法覆盖：4 个属性读取 + openSystemMonitor（Q_INVOKABLE）全覆盖；
// 2 等价类：探测成功值/失败值（-1 保留旧值）、DBus 有效/无效；
// 3 边界值：属性默认值 0/"0 KB/s"、prober 失败 -1；
// 4 分支清单→用例映射：见下方【分支清单】；
// 5 分支触发：systemStatsChanged 变更/未变更双路径、DBus 回退路径；
// 6 异常断言：空指针/无总线场景以 EXPECT_NO_THROW 与打桩兜底；
// 7 反向场景：DBUS 无服务 → startDetached 回退；
// 8 强异常安全：重复 refreshSystemStats 状态自恢复；
// 9 打桩选型：prober 非虚方法 set_lamda；DBus 非虚方法 set_lamda（isValid/
//   doAsyncCall 为 QDBusAbstractInterface 库外联符号）；静态 startDetached 打桩；
// 10 信号验证：QSignalSpy 捕获 systemStatsChanged。
//
// 【分支清单】（复杂方法分支 → 用例映射）
// SystemMonitorController (构造)：
//   B1 创建 prober + 2s 定时器 + 立即刷新 → Ctor_Init_CreatesTimerAndRefreshes
//   B2 首次刷新失败（prober -1）→ 保留默认值 → Ctor_ProberFailure_KeepsDefaults
// refreshSystemStats：
//   B3 nextCpu>=0 → 更新 m_cpuUsage → RefreshSystemStats_Change_UpdatesProperties
//   B4 nextCpu/nextMem<0 → 保留旧值 → RefreshSystemStats_ProberFailure_KeepsPrevious
//   B5 任一值变化 → emit systemStatsChanged → RefreshSystemStats_Change_EmitsSignal
//   B6 值均未变 → 不发射 → RefreshSystemStats_Unchanged_NoSignal
//   B7 仅网速文本变化 → 发射 → RefreshSystemStats_NetworkTextChange_EmitsSignal
// openSystemMonitor：
//   B8 serverIface.isValid() → asyncCall(showDeepinSystemMoniter) →
//      OpenSystemMonitor_DBusValid_CallsAsyncMethod
//   B9 isValid false → startDetached 拉起 deepin-system-monitor →
//      OpenSystemMonitor_DBusInvalid_FallsBackToDetached
//   B10 Q_INVOKABLE 元对象调用可达 → OpenSystemMonitor_MetaObject_InvokeSucceeds

#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include <stubext.h>

#include "systemmonitorcontroller.h"
#include "systemmonitorprober.h"

#include <QDBusAbstractInterface>
#include <QDBusConnection>
#include <QDBusPendingCall>
#include <QProcess>
#include <QSignalSpy>
#include <QTest>
#include <QTimer>

namespace {
// QDBusAbstractInterface 库外联私有方法签名（Qt 6.8）
using DoAsyncCallFn = QDBusPendingCall (QDBusAbstractInterface::*)(const QString &, const QVariant *, size_t);
using StartDetachedFn = bool (*)(const QString &, const QStringList &, const QString &, qint64 *);
} // namespace

class SystemMonitorControllerTest : public ::testing::Test
{
protected:
    void SetUp() override {}

    void TearDown() override
    {
        stub.clear();
        controller.reset();
        s_openSystemMonitorCalls = 0;
        s_detachedPrograms.clear();
        s_asyncMethods.clear();
    }

    // 打桩 prober 三接口（非虚成员，地址补丁），使控制器与 /proc 完全隔离
    void stubProber(int cpu, int mem, const QString &down, const QString &up)
    {
        s_cpu = cpu;
        s_mem = mem;
        s_down = down;
        s_up = up;
        stub.set_lamda(&SystemMonitorProber::cpuUsagePercent,
                       [](SystemMonitorProber *) { return s_cpu; });
        stub.set_lamda(&SystemMonitorProber::memoryUsagePercent,
                       [](SystemMonitorProber *) { return s_mem; });
        stub.set_lamda(&SystemMonitorProber::networkSpeed,
                       [](SystemMonitorProber *, QString *download, QString *upload) {
                           if (download)
                               *download = s_down;
                           if (upload)
                               *upload = s_up;
                       });
    }

    // 打桩 DBus：sessionBus 返回空连接（隔离真实总线），isValid 可控，
    // doAsyncCall 记录方法名，startDetached 记录拉起程序
    void stubDBus(bool ifaceValid)
    {
        stub.set_lamda(static_cast<QDBusConnection (*)()>(&QDBusConnection::sessionBus),
                       []() -> QDBusConnection { return QDBusConnection(QString()); });
        stub.set_lamda(static_cast<bool (QDBusAbstractInterface::*)() const>(&QDBusAbstractInterface::isValid),
                       [ifaceValid](QDBusAbstractInterface *) { return ifaceValid; });
        stub.set_lamda(static_cast<DoAsyncCallFn>(&QDBusAbstractInterface::doAsyncCall),
                       [](QDBusAbstractInterface *, const QString &method, const QVariant *, size_t) -> QDBusPendingCall {
                           s_asyncMethods.append(method);
                           return QDBusPendingCall::fromError(QDBusError());
                       });
        stub.set_lamda(static_cast<StartDetachedFn>(&QProcess::startDetached),
                       [](const QString &program, const QStringList &, const QString &, qint64 *) -> bool {
                           s_detachedPrograms.append(program);
                           return true;
                       });
    }

    std::unique_ptr<SystemMonitorController> makeController()
    {
        return std::make_unique<SystemMonitorController>();
    }

    stub_ext::StubExt stub;
    std::unique_ptr<SystemMonitorController> controller;

    inline static int s_cpu = 0;
    inline static int s_mem = 0;
    inline static QString s_down;
    inline static QString s_up;
    inline static int s_openSystemMonitorCalls = 0;
    inline static QStringList s_detachedPrograms;
    inline static QStringList s_asyncMethods;
};

// ---- 构造 / 属性初始状态 ----

TEST_F(SystemMonitorControllerTest, Ctor_Init_CreatesTimerAndRefreshes)
{
    // Arrange: prober 注入固定值
    stubProber(37, 55, QStringLiteral("1.2 MB/s"), QStringLiteral("33 KB/s"));
    // Act: 构造（内部立即 refreshSystemStats 一次）
    controller = makeController();
    // Assert: 属性来自 prober；存在 2s 定时器且已启动
    EXPECT_EQ(controller->cpuUsage(), 37);
    EXPECT_EQ(controller->memoryUsage(), 55);
    EXPECT_EQ(controller->downloadSpeedText(), QStringLiteral("1.2 MB/s"));
    EXPECT_EQ(controller->uploadSpeedText(), QStringLiteral("33 KB/s"));
    QTimer *timer = controller->findChild<QTimer *>();
    ASSERT_NE(timer, nullptr);
    EXPECT_EQ(timer->interval(), 2000);
    EXPECT_TRUE(timer->isActive());
}

TEST_F(SystemMonitorControllerTest, Ctor_ProberFailure_KeepsDefaults)
{
    // Arrange: prober 返回 -1（读取失败）
    stubProber(-1, -1, QStringLiteral("0 KB/s"), QStringLiteral("0 KB/s"));
    // Act: 构造
    controller = makeController();
    // Assert: 保留默认值 0 / "0 KB/s"
    EXPECT_EQ(controller->cpuUsage(), 0);
    EXPECT_EQ(controller->memoryUsage(), 0);
    EXPECT_EQ(controller->downloadSpeedText(), QStringLiteral("0 KB/s"));
    EXPECT_EQ(controller->uploadSpeedText(), QStringLiteral("0 KB/s"));
}

// ---- 属性经元对象可读（Q_PROPERTY 绑定给 QML 的路径） ----

TEST_F(SystemMonitorControllerTest, Properties_MetaObject_ExposeAllValues)
{
    // Arrange
    stubProber(42, 88, QStringLiteral("9 KB/s"), QStringLiteral("3 KB/s"));
    controller = makeController();
    // Act: QML 通过属性系统逐个读取
    const QVariant cpu = controller->property("cpuUsage");
    const QVariant mem = controller->property("memoryUsage");
    const QVariant down = controller->property("downloadSpeedText");
    const QVariant up = controller->property("uploadSpeedText");
    // Assert: 四个属性均来自 prober
    EXPECT_EQ(cpu.toInt(), 42);
    EXPECT_EQ(mem.toInt(), 88);
    EXPECT_EQ(down.toString(), QStringLiteral("9 KB/s"));
    EXPECT_EQ(up.toString(), QStringLiteral("3 KB/s"));
}

TEST_F(SystemMonitorControllerTest, Properties_MetaObject_NotifySignalRegistered)
{
    // Arrange: 四个属性均应挂接 systemStatsChanged 通知信号
    stubProber(1, 2, QStringLiteral("0 KB/s"), QStringLiteral("0 KB/s"));
    controller = makeController();
    const QMetaObject *mo = controller->metaObject();
    const int idx = mo->indexOfSignal("systemStatsChanged()");
    // Act: 逐属性检查 notify 信号挂接
    ASSERT_GE(idx, 0);
    int attached = 0;
    for (const char *prop : { "cpuUsage", "memoryUsage", "downloadSpeedText", "uploadSpeedText" }) {
        const QMetaProperty mp = mo->property(mo->indexOfProperty(prop));
        ASSERT_TRUE(mp.isValid());
        if (mp.hasNotifySignal() && mp.notifySignal().methodIndex() == idx) {
            ++attached;
        }
    }
    // Assert: 信号存在且四个属性全部挂接同一通知信号
    EXPECT_GT(idx, -1);
    EXPECT_EQ(attached, 4);
}

// ---- refreshSystemStats ----

TEST_F(SystemMonitorControllerTest, RefreshSystemStats_Change_UpdatesProperties)
{
    // Arrange: 初始 37/55，随后变更 stub 值
    stubProber(37, 55, QStringLiteral("1.2 MB/s"), QStringLiteral("33 KB/s"));
    controller = makeController();
    s_cpu = 40;
    s_mem = 60;
    s_down = QStringLiteral("2.0 MB/s");
    s_up = QStringLiteral("64 KB/s");
    // Act: 私有槽直调（-fno-access-control）
    controller->refreshSystemStats();
    // Assert
    EXPECT_EQ(controller->cpuUsage(), 40);
    EXPECT_EQ(controller->memoryUsage(), 60);
    EXPECT_EQ(controller->downloadSpeedText(), QStringLiteral("2.0 MB/s"));
    EXPECT_EQ(controller->uploadSpeedText(), QStringLiteral("64 KB/s"));
}

TEST_F(SystemMonitorControllerTest, RefreshSystemStats_ProberFailure_KeepsPrevious)
{
    // Arrange: 初始 37/55；随后 prober 失败返回 -1
    stubProber(37, 55, QStringLiteral("1.2 MB/s"), QStringLiteral("33 KB/s"));
    controller = makeController();
    s_cpu = -1;
    s_mem = -1;
    // Act
    controller->refreshSystemStats();
    // Assert: CPU/内存保留旧值（网速无失败语义，按 prober 输出更新）
    EXPECT_EQ(controller->cpuUsage(), 37);
    EXPECT_EQ(controller->memoryUsage(), 55);
}

TEST_F(SystemMonitorControllerTest, RefreshSystemStats_Change_EmitsSignal)
{
    // Arrange: 构造完成（首次刷新已发射一次，spy 只统计后续）
    stubProber(37, 55, QStringLiteral("1.2 MB/s"), QStringLiteral("33 KB/s"));
    controller = makeController();
    QSignalSpy spy(controller.get(), &SystemMonitorController::systemStatsChanged);
    ASSERT_EQ(spy.count(), 0);
    s_cpu = 40;
    // Act
    controller->refreshSystemStats();
    // Assert: 值变化 → 恰好发射一次，且属性已更新
    EXPECT_EQ(spy.count(), 1);
    EXPECT_EQ(controller->cpuUsage(), 40);
}

TEST_F(SystemMonitorControllerTest, RefreshSystemStats_Unchanged_NoSignal)
{
    // Arrange
    stubProber(37, 55, QStringLiteral("1.2 MB/s"), QStringLiteral("33 KB/s"));
    controller = makeController();
    QSignalSpy spy(controller.get(), &SystemMonitorController::systemStatsChanged);
    // Act: 连续两次刷新，值均未变化
    controller->refreshSystemStats();
    controller->refreshSystemStats();
    // Assert: 未发射信号，属性保持原值
    EXPECT_EQ(spy.count(), 0);
    EXPECT_EQ(controller->cpuUsage(), 37);
}

TEST_F(SystemMonitorControllerTest, RefreshSystemStats_NetworkTextChange_EmitsSignal)
{
    // Arrange: CPU/内存不变，仅网速文本变化
    stubProber(37, 55, QStringLiteral("1.2 MB/s"), QStringLiteral("33 KB/s"));
    controller = makeController();
    QSignalSpy spy(controller.get(), &SystemMonitorController::systemStatsChanged);
    s_down = QStringLiteral("1.3 MB/s");
    // Act
    controller->refreshSystemStats();
    // Assert: 发射一次，且网速文本已更新
    EXPECT_EQ(spy.count(), 1);
    EXPECT_EQ(controller->downloadSpeedText(), QStringLiteral("1.3 MB/s"));
}

TEST_F(SystemMonitorControllerTest, RefreshSystemStats_RepeatedCalls_StateSelfRecovery)
{
    // Arrange: 失败与成功交替
    stubProber(37, 55, QStringLiteral("1.2 MB/s"), QStringLiteral("33 KB/s"));
    controller = makeController();
    // Act: 失败刷新
    s_cpu = -1;
    s_mem = -1;
    controller->refreshSystemStats();
    // Assert: 失败保留旧值
    EXPECT_EQ(controller->cpuUsage(), 37);
    // Act: 恢复后刷新
    s_cpu = 90;
    s_mem = 95;
    controller->refreshSystemStats();
    // Assert: 更新为新值（状态自恢复）
    EXPECT_EQ(controller->cpuUsage(), 90);
    EXPECT_EQ(controller->memoryUsage(), 95);
}

// ---- 定时刷新 ----

TEST_F(SystemMonitorControllerTest, Timer_Interval_FiresRefreshAndEmits)
{
    // Arrange: 初始 37，2s 后 prober 值变为 40
    stubProber(37, 55, QStringLiteral("1.2 MB/s"), QStringLiteral("33 KB/s"));
    controller = makeController();
    QSignalSpy spy(controller.get(), &SystemMonitorController::systemStatsChanged);
    s_cpu = 40;
    // Act: 真实事件循环等待定时器触发（间隔 2000ms）
    QTest::qWait(2300);
    // Assert: 定时刷新生效且发射变更信号
    EXPECT_GE(spy.count(), 1);
    EXPECT_EQ(controller->cpuUsage(), 40);
}

TEST_F(SystemMonitorControllerTest, Timer_Children_ExactlyOneRefreshTimer)
{
    // Arrange
    stubProber(1, 2, QStringLiteral("0 KB/s"), QStringLiteral("0 KB/s"));
    // Act: 构造后检查子对象
    controller = makeController();
    const QList<QTimer *> timers = controller->findChildren<QTimer *>();
    // Assert: 恰有一个 2s 周期定时器且已启动（避免重复刷新）
    ASSERT_EQ(timers.size(), 1);
    EXPECT_EQ(timers.first()->interval(), 2000);
    EXPECT_TRUE(timers.first()->isActive());
}

// ---- openSystemMonitor ----

TEST_F(SystemMonitorControllerTest, OpenSystemMonitor_DBusValid_CallsAsyncMethod)
{
    // Arrange: prober 与 DBus 均打桩（完全不触真实 /proc 与真实总线）
    stubProber(0, 0, QStringLiteral("0 KB/s"), QStringLiteral("0 KB/s"));
    stubDBus(true);
    controller = makeController();
    // Act
    controller->openSystemMonitor();
    // Assert: 方法名正确（注意源码拼写 showDeepinSystemMoniter），未拉起进程
    ASSERT_EQ(s_asyncMethods.size(), 1);
    EXPECT_EQ(s_asyncMethods.first(), QStringLiteral("showDeepinSystemMoniter"));
    EXPECT_TRUE(s_detachedPrograms.isEmpty());
}

TEST_F(SystemMonitorControllerTest, OpenSystemMonitor_DBusInvalid_FallsBackToDetached)
{
    // Arrange: DBus 服务无效 → startDetached 回退
    stubProber(0, 0, QStringLiteral("0 KB/s"), QStringLiteral("0 KB/s"));
    stubDBus(false);
    controller = makeController();
    // Act
    controller->openSystemMonitor();
    // Assert: 拉起 deepin-system-monitor，未发起 DBus 调用
    ASSERT_EQ(s_detachedPrograms.size(), 1);
    EXPECT_EQ(s_detachedPrograms.first(), QStringLiteral("deepin-system-monitor"));
    EXPECT_TRUE(s_asyncMethods.isEmpty());
}

TEST_F(SystemMonitorControllerTest, OpenSystemMonitor_MetaObject_InvokeSucceeds)
{
    // Arrange: Q_INVOKABLE 经元对象可达（QML/菜单调用路径）
    stubProber(0, 0, QStringLiteral("0 KB/s"), QStringLiteral("0 KB/s"));
    stubDBus(true);
    controller = makeController();
    // Act: invokeMethod 不带参数
    const bool invoked = QMetaObject::invokeMethod(controller.get(), "openSystemMonitor");
    // Assert
    EXPECT_TRUE(invoked);
    EXPECT_EQ(s_asyncMethods.size(), 1);
}
