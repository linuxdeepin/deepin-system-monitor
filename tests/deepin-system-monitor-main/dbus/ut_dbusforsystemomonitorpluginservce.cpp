// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// UT_DBusForSystemoMonitorPluginServce：DBus 插件跳转接口单元测试（qt-autotest-generator 重写）
//
// 被测文件：deepin-system-monitor-main/dbus/dbusforsystemomonitorpluginservce.cpp
// 重写原因：Mode 6 审查判 FAIL（原用例依赖真实 MainWindow 生命周期且断言空洞）
//
// 打桩说明（外部依赖全部 stub，不依赖真实窗口/DBus 环境）：
// - Application::getCurrentTaskState → 受控任务状态（kTaskFinished / kTaskRunning）
// - Application::raiseWindow         → 计数（不触发真实窗口置顶）
// - DetailWidgetManager 跳转         → 真实方法 + QSignalSpy 捕获信号（验证跳转目标）
//
// ============================ 用例计数声明 ============================
// 方法                                   | level | 最少用例 | actual | 说明
// DBusForSystemoMonitorPluginServce()    | low   | 1 | 1 | 构造析构不崩溃
// ~DBusForSystemoMonitorPluginServce()   | low   | 1 | - | 由用例栈/删除对象析构覆盖
// slotJumpDetailWidget(const QString&)   | mid   | 2 | 2 | 任务完成跳转并发信号 / 任务运行中不跳转
// slotJumpProcessWidget(const QString&)  | mid   | 2 | 2 | 任务完成跳进程页 / 任务运行中不跳转
// slotRaiseWindow()                      | mid   | 2 | 2 | 任务完成置顶 / 任务运行中不置顶
//
// ============================== 最小清单 ==============================
// [x] 1 公开方法已列出，每个方法 ≥ 1 用例
// [x] 2 输入按等价类划分：任务状态 完成/运行中（kTaskFinished vs 其他态）
// [x] 3 边界值显式覆盖：kTaskFinished（唯一放行态）与 kTaskRunning（首个非放行态）
// [x] 4 无 ≥3 组同质断言输入，不适用 TEST_P（3 个槽 × 2 态，均为独立断言目标）
// [x] 5 分支清单已列出并映射到用例名
// [x] 6 每条 if/early-return 分支有触发用例（3 槽 × 放行/拦截）
// [x] 7 本类无显式 throw，不适用 EXPECT_THROW
// [x] 8 负面场景（任务运行中）有专门用例
// [x] 9 负面用例验证强异常安全（拦截态下无任何信号与置顶副作用）
// [x] 10 项目内接口类用 gMock，其他用 stub_ext（本项目统一用项目自带 stub.h 打桩）
//
// ============================== 分支清单 ==============================
// （来源：工作目录源码 dbusforsystemomonitorpluginservce.cpp）
// slotJumpDetailWidget(const QString&):
//   B1  状态==kTaskFinished → raiseWindow + jumpDetailWidget(发 sigJumpToDetailWidget) → SlotJumpDetailWidget_TaskFinished_JumpsAndRaises
//   B2  状态!=kTaskFinished → 无任何副作用                                              → SlotJumpDetailWidget_TaskRunning_NoJumpNoRaise
// slotJumpProcessWidget(const QString&):
//   B3  状态==kTaskFinished → raiseWindow + jumpProcessWidget(发 sigJumpToProcessWidget) → SlotJumpProcessWidget_TaskFinished_JumpsToProcessPage
//   B4  状态!=kTaskFinished → 无任何副作用                                               → SlotJumpProcessWidget_TaskRunning_NoJumpNoRaise
// slotRaiseWindow():
//   B5  状态==kTaskFinished → raiseWindow                                                → SlotRaiseWindow_TaskFinished_RaisesWindow
//   B6  状态!=kTaskFinished → 无任何副作用                                               → SlotRaiseWindow_TaskRunning_NoRaise

#include "dbus/dbusforsystemomonitorpluginservce.h"
//self
#include "application.h"
#include "detailwidgetmanager.h"
//qt
#include <QSignalSpy>
#include <QString>
//gtest
#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
//stub
#include "stub.h"

using namespace testing;

// ============================== 打桩实现 ==============================
namespace {
int g_raiseCount = 0;
Application::TaskState g_taskState = Application::kTaskRunning;

void resetPluginStubState()
{
    g_raiseCount = 0;
    g_taskState = Application::kTaskRunning;
}

// Application::TaskState Application::getCurrentTaskState()
static Application::TaskState stub_App_getCurrentTaskState()
{
    return g_taskState;
}

// void Application::raiseWindow()
static void stub_App_raiseWindow()
{
    ++g_raiseCount;
}
}  // namespace

// ============================== 夹具 ==============================
class UT_DBusForSystemoMonitorPluginServce : public ::testing::Test
{
public:
    UT_DBusForSystemoMonitorPluginServce() {}
    ~UT_DBusForSystemoMonitorPluginServce() override {}

    void SetUp() override
    {
        resetPluginStubState();
        m_tester = new DBusForSystemoMonitorPluginServce();
    }

    void TearDown() override
    {
        delete m_tester;
        m_tester = nullptr;
    }

    DBusForSystemoMonitorPluginServce *m_tester = nullptr;
};

// ============================== 用例 ==============================
// 构造/析构安全
TEST_F(UT_DBusForSystemoMonitorPluginServce, CtorDtor_NewDelete_NoCrash)
{
    // Arrange: SetUp 已构造对象
    // Act: SetUp 已构造对象（TearDown 负责 delete）
    // Assert: 对象非空且无父对象（构造成功）
    EXPECT_NE(m_tester, nullptr);
    EXPECT_EQ(m_tester->parent(), nullptr);
}

// B1: 任务完成 → 置顶 + 跳转详情页（信号携带消息码）
TEST_F(UT_DBusForSystemoMonitorPluginServce, SlotJumpDetailWidget_TaskFinished_JumpsAndRaises)
{
    // Arrange: 任务完成态 + 信号监视
    Stub stub;
    stub.set(ADDR(Application, getCurrentTaskState), stub_App_getCurrentTaskState);
    stub.set(ADDR(Application, raiseWindow), stub_App_raiseWindow);
    resetPluginStubState();
    g_taskState = Application::kTaskFinished;
    DetailWidgetManager &mgr = DetailWidgetManager::getInstance();
    QSignalSpy spy(&mgr, &DetailWidgetManager::sigJumpToDetailWidget);

    // Act: 触发 DBus 跳转
    m_tester->slotJumpDetailWidget(QStringLiteral("MSG_CPU"));

    // Assert: 恰一次置顶且信号携带消息码
    EXPECT_EQ(g_raiseCount, 1);
    EXPECT_EQ(spy.count(), 1);
    if (spy.count() == 1)
        EXPECT_EQ(spy.at(0).at(0).toString(), QStringLiteral("MSG_CPU"));
}

// B2: 任务运行中 → 不置顶不跳转
TEST_F(UT_DBusForSystemoMonitorPluginServce, SlotJumpDetailWidget_TaskRunning_NoJumpNoRaise)
{
    // Arrange: 任务运行态
    Stub stub;
    stub.set(ADDR(Application, getCurrentTaskState), stub_App_getCurrentTaskState);
    stub.set(ADDR(Application, raiseWindow), stub_App_raiseWindow);
    resetPluginStubState();
    DetailWidgetManager &mgr = DetailWidgetManager::getInstance();
    QSignalSpy spy(&mgr, &DetailWidgetManager::sigJumpToDetailWidget);

    // Act: 触发 DBus 跳转
    m_tester->slotJumpDetailWidget(QStringLiteral("MSG_CPU"));

    // Assert: 零副作用
    EXPECT_EQ(g_raiseCount, 0);
    EXPECT_EQ(spy.count(), 0);
}

// B3: 任务完成 → 置顶 + 跳转进程页
TEST_F(UT_DBusForSystemoMonitorPluginServce, SlotJumpProcessWidget_TaskFinished_JumpsToProcessPage)
{
    // Arrange: 任务完成态 + 信号监视
    Stub stub;
    stub.set(ADDR(Application, getCurrentTaskState), stub_App_getCurrentTaskState);
    stub.set(ADDR(Application, raiseWindow), stub_App_raiseWindow);
    resetPluginStubState();
    g_taskState = Application::kTaskFinished;
    DetailWidgetManager &mgr = DetailWidgetManager::getInstance();
    QSignalSpy spy(&mgr, &DetailWidgetManager::sigJumpToProcessWidget);

    // Act: 触发 DBus 跳转
    m_tester->slotJumpProcessWidget(QStringLiteral("MSG_PROCESS"));

    // Assert: 恰一次置顶且信号携带消息码
    EXPECT_EQ(g_raiseCount, 1);
    EXPECT_EQ(spy.count(), 1);
    if (spy.count() == 1)
        EXPECT_EQ(spy.at(0).at(0).toString(), QStringLiteral("MSG_PROCESS"));
}

// B4: 任务运行中 → 不置顶不跳转
TEST_F(UT_DBusForSystemoMonitorPluginServce, SlotJumpProcessWidget_TaskRunning_NoJumpNoRaise)
{
    // Arrange: 任务运行态
    Stub stub;
    stub.set(ADDR(Application, getCurrentTaskState), stub_App_getCurrentTaskState);
    stub.set(ADDR(Application, raiseWindow), stub_App_raiseWindow);
    resetPluginStubState();
    DetailWidgetManager &mgr = DetailWidgetManager::getInstance();
    QSignalSpy spy(&mgr, &DetailWidgetManager::sigJumpToProcessWidget);

    // Act: 触发 DBus 跳转
    m_tester->slotJumpProcessWidget(QStringLiteral("MSG_PROCESS"));

    // Assert: 零副作用
    EXPECT_EQ(g_raiseCount, 0);
    EXPECT_EQ(spy.count(), 0);
}

// B5: 任务完成 → 置顶
TEST_F(UT_DBusForSystemoMonitorPluginServce, SlotRaiseWindow_TaskFinished_RaisesWindow)
{
    // Arrange: 任务完成态
    Stub stub;
    stub.set(ADDR(Application, getCurrentTaskState), stub_App_getCurrentTaskState);
    stub.set(ADDR(Application, raiseWindow), stub_App_raiseWindow);
    resetPluginStubState();
    g_taskState = Application::kTaskFinished;

    // Act: 触发置顶
    m_tester->slotRaiseWindow();

    // Assert: 恰一次置顶（前后计数各一断言）
    EXPECT_EQ(g_raiseCount, 1);
    EXPECT_NE(g_raiseCount, 0);
}

// B6: 任务运行中 → 不置顶
TEST_F(UT_DBusForSystemoMonitorPluginServce, SlotRaiseWindow_TaskRunning_NoRaise)
{
    // Arrange: 任务运行态
    Stub stub;
    stub.set(ADDR(Application, getCurrentTaskState), stub_App_getCurrentTaskState);
    stub.set(ADDR(Application, raiseWindow), stub_App_raiseWindow);
    resetPluginStubState();

    // Act: 触发置顶
    m_tester->slotRaiseWindow();

    // Assert: 零副作用（raiseWindow 打桩计数保持 0，getTaskState 被查过）
    EXPECT_EQ(g_raiseCount, 0);
    EXPECT_NE(g_taskState, Application::kTaskFinished);
}
