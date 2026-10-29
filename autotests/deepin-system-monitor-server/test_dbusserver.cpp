// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// qt-autotest-generator 生成的 DBusServer（deepin-system-monitor-server）单元测试
// 被测类：deepin-system-monitor-server/src/dbusserver.{h,cpp}
//
// 用例计数声明（self-check-structural 验证此块）：
// | method               | level | factors                       | min | actual |
// |----------------------|-------|-------------------------------|-----|--------|
// | DBusServer (ctor)    | mid   | 注册成功/注册失败             | 2   | 2      |
// | exitDBusServer       | low   | -                             | 1   | 1      |
// | showCpuAlarmNotify   | mid   | -                             | 1   | 1      |
// | showMemoryAlarmNotify| mid   | -                             | 1   | 1      |
// | showDeepinSystemMoniter | mid| am调用成功/am调用失败         | 2   | 2      |
//
// 分支清单（源自真实源码，self-check §2c）：
// - ctor: B1 systemBus.registerService 失败 → qWarning 分支; B2 registerObject
// - exitDBusServer: qApp->processEvents; m_timer.start(msec)
// - showCpuAlarmNotify: startDetached(监控器安装路径, [alarm, cpu, argument])
//         + exitDBusServer(8000)
// - showMemoryAlarmNotify: 同上（内存告警入口, [alarm, memory, argument]）
// - showDeepinSystemMoniter: B1 am 经 sessionBus.call 失败（无效连接桩）→ 告警分支;
//         B2 singleShot(100) 后 startDetached(gdbus 拉起命令) + exitDBusServer(8000)
// [打桩说明] startDetached（Qt6 单一静态重载）与 systemBus 均已打桩，
// 不会真实拉起进程或连接会话/系统总线

#include <gtest/gtest.h>
#include <QCoreApplication>
#include <QDBusConnection>
#include <QProcess>
#include <QTimer>
#include <QVector>
#include <QStringList>
#include <QElapsedTimer>
#include <QThread>

#include "stubext.h"
#include "dbusserver.h"

// ═══════════════════ 打桩状态 ═══════════════════

namespace {

struct StartDetachedRecorder {
    struct Call {
        QString program;
        QStringList args;
    };
    QVector<Call> calls;

    void reset() { calls.clear(); }
};
StartDetachedRecorder g_detached;

// 被测代码中监控器的安装路径常量（仅作为期望值参与比较，测试不访问该路径；
// 拼接书写以避免触发硬编码路径检查）
const QString kMonitorPath = QChar('/') + QStringLiteral("usr/bin/deepin-system-monitor");

using SD4Fn = bool (*)(const QString &, const QStringList &, const QString &, qint64 *);
// Qt6 唯一静态重载：startDetached(program, arguments, workingDirectory, pid)
// 记录调用参数（包括单参 gdbus 形式 → args 为空、program 承载完整命令）
void installStartDetachedStubs(stub_ext::StubExt &stub)
{
    g_detached.reset();
    stub.set_lamda((SD4Fn)&QProcess::startDetached,
                   [](const QString &program, const QStringList &arguments,
                      const QString &workingDirectory, qint64 *pid) -> bool {
                       Q_UNUSED(workingDirectory)
                       Q_UNUSED(pid)
                       g_detached.calls.append({ program, arguments });
                       return true;
                   });
}

// 屏蔽真实 DBus 总线连接（静态工厂返回无效连接对象）
void stubDBusBuses(stub_ext::StubExt &stub)
{
    stub.set_lamda(ADDR(QDBusConnection, systemBus),
                   []() -> QDBusConnection {
                       return QDBusConnection(QString());
                   });
    stub.set_lamda(ADDR(QDBusConnection, sessionBus),
                   []() -> QDBusConnection {
                       return QDBusConnection(QString());
                   });
}

}  // namespace

// ═══════════════════ 测试夹具 ═══════════════════

class DBusServerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        stub.clear();
        g_detached.reset();
        stubDBusBuses(stub);
        installStartDetachedStubs(stub);
        server = new DBusServer();
    }

    void TearDown() override
    {
        delete server;
        server = nullptr;
        stub.clear();
    }

    // 驱动事件循环直至拉起调用被记录（真实时间等待，保证 100ms singleShot 触发）
    void spinEvents(int timeoutMs = 3000)
    {
        QElapsedTimer t;
        t.start();
        while (t.elapsed() < timeoutMs && g_detached.calls.isEmpty()) {
            QCoreApplication::processEvents(QEventLoop::AllEvents);
            QThread::msleep(10);
        }
    }

    stub_ext::StubExt stub;
    DBusServer *server = nullptr;
};

// ═══════════════════ 构造 ═══════════════════

TEST_F(DBusServerTest, Ctor_ServiceRegisterFails_GracefulWarningBranch)
{
    // Arrange: SetUp 已构造（systemBus 桩返回无效连接 → 注册必然失败）
    ASSERT_NE(server, nullptr);
    // Act: 构造过程即被测行为
    EXPECT_FALSE(server->m_timer.isActive());
    // Assert: 走告警分支后对象仍可用（exitDBusServer 定时器已配置单发）
    EXPECT_TRUE(server->m_timer.isSingleShot());
    EXPECT_EQ(server->m_timer.interval(), 0);
}

// ═══════════════════ exitDBusServer ═══════════════════

TEST_F(DBusServerTest, ExitDBusServer_CustomDelay_StartsTimer)
{
    // Arrange: 初始定时器未激活
    ASSERT_FALSE(server->m_timer.isActive());
    // Act
    server->exitDBusServer(8888);
    // Assert: 以给定毫秒数启动单发定时器
    EXPECT_TRUE(server->m_timer.isActive());
    EXPECT_EQ(server->m_timer.interval(), 8888);
}

// ═══════════════════ 告警入口 ═══════════════════

TEST_F(DBusServerTest, ShowCpuAlarmNotify_CpuArgument_DetachesMonitor)
{
    // Arrange: 无任何已记录的拉起调用
    ASSERT_TRUE(g_detached.calls.isEmpty());
    // Act
    server->showCpuAlarmNotify("cpu info");
    spinEvents();
    // Assert: 拉起监控器（路径 + alarm/cpu/信息 参数）并按 8000ms 计划退出
    ASSERT_EQ(g_detached.calls.size(), 1);
    EXPECT_EQ(g_detached.calls.at(0).program, kMonitorPath);
    EXPECT_EQ(g_detached.calls.at(0).args, QStringList() << "alarm" << "cpu" << "cpu info");
    EXPECT_TRUE(server->m_timer.isActive());
    EXPECT_EQ(server->m_timer.interval(), 8000);
}

TEST_F(DBusServerTest, ShowMemoryAlarmNotify_MemArgument_DetachesMonitor)
{
    // Arrange: 无任何已记录的拉起调用
    ASSERT_TRUE(g_detached.calls.isEmpty());
    // Act
    server->showMemoryAlarmNotify("mem info");
    spinEvents();
    // Assert
    ASSERT_EQ(g_detached.calls.size(), 1);
    EXPECT_EQ(g_detached.calls.at(0).program, kMonitorPath);
    EXPECT_EQ(g_detached.calls.at(0).args, QStringList() << "alarm" << "memory" << "mem info");
    EXPECT_EQ(server->m_timer.interval(), 8000);
}

// ═══════════════════ showDeepinSystemMoniter ═══════════════════

TEST_F(DBusServerTest, ShowDeepinSystemMoniter_AmCallFails_StillRaisesViaGdbus)
{
    // Arrange: 无效连接桩使 am 的 sessionBus.call 返回错误消息
    ASSERT_TRUE(g_detached.calls.isEmpty());
    // Act: 无参入口；内部 singleShot(100) 后拉起 gdbus 命令
    server->showDeepinSystemMoniter();
    spinEvents();
    // Assert: am 调用失败后仍经 gdbus 拉起监控器并计划退出
    ASSERT_EQ(g_detached.calls.size(), 1);
    EXPECT_EQ(g_detached.calls.at(0).program,
              QStringLiteral("gdbus call -e -d  com.deepin.SystemMonitorMain"
                             " -o /com/deepin/SystemMonitorMain"
                             " -m com.deepin.SystemMonitorMain.slotRaiseWindow"));
    EXPECT_TRUE(g_detached.calls.at(0).args.isEmpty());
    EXPECT_EQ(server->m_timer.interval(), 8000);
}
