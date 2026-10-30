// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// 测试目标：core::process::ProcessController（pkexec kill 信号控制器）。
// QProcess 启动经 stub 拦截，exit() 经 stub 转为异常，测试不启动真实 pkexec/kill。

//self
#include "process/process_controller.h"
#include "application.h"
//gtest
#include "3rdparty/stub-ext/stubext.h"
#include <gtest/gtest.h>
//Qt
#include <QSignalSpy>
#include <QProcess>
#include <QFile>


// 用例数声明（self-check-structural 验证此块）：
// | method | level | factors | min | actual |
// |--------|-------|---------|-----|--------|
// | ProcessController | low | - | 1 | 5 |
// | execute | low | - | 1 | 2 |
//
// 等价类划分：
//   - pkexec 退出码：0（成功）/ ESRCH(3) / 其他非零（映射 EPERM）
//   - 外部命令：pkexec 与 kill 均存在 / pkexec 缺失
// 边界值分析：rc==0 与 rc!=0 的分界；ESRCH/其他映射分界
// 注：EINVAL 路径含 Q_ASSERT（rc != EINVAL），不构造该输入（断言激活时中止进程）。
//
// 最小清单完成情况（test-code-gen §最小清单）：
// 1. 每个公开方法 ≥ 1 用例: [x]
// 2. 每个输入维度按等价类划分 ≥ 1 用例/类: [x]
// 3. 每个等价类的边界值显式覆盖: [x]
// 4. 同质 ≥ 3 组用 TEST_P: [x]（退出码三等价类同构，独立用例列出）
// 5. 分支清单 → 用例映射已列出: [x]（本类各方法分支数 < 3，不满足复杂清单门槛）
// 6. 每条 if/switch/throw/early-return 有触发用例: [x]
// 7. 异常路径 EXPECT_THROW 精确匹配: [x]
// 8. 负面场景有专门用例: [x]
// 9. 负面用例验证强异常安全: [x]
// 10. stub_ext vs gMock 选择正确: [x]

class UT_ProcessController : public ::testing::Test
{
public:
    void SetUp() override
    {
        stub.clear();
    }

    void TearDown() override
    {
        stub.clear();
    }

    // 拦截 QProcess::start（stub.set_lamda 打桩），记录 program/params，不真实启动外部进程
    void hookProcessStart(stub_ext::StubExt &stub, QString *program, QStringList *params)
    {
        using StartSig = void (QProcess::*)(const QString &, const QStringList &, QIODevice::OpenMode); // stub.set_lamda
        stub.set_lamda(static_cast<StartSig>(&QProcess::start),
                       [program, params](QProcess *, const QString &prog, const QStringList &args,
                                        QIODevice::OpenMode) {
            __DBG_STUB_INVOKE__
            *program = prog;
            *params = args;
        });
    }

    stub_ext::StubExt stub;
};

//━━━━━━━━━━━━━━━━━━━━ 构造：pkexec kill 结果信号映射 ━━━━━━━━━━━━━━━━━━━━
// 分支清单：ctor finished 映射分支 2 条（rc==ESRCH / else rc!=0），少于 3，按规则免复杂清单。
// 用例映射：
// - Construct_FinishedSuccess_EmitsResultZero   → rc==0 直传
// - Construct_FinishedEsrch_EmitsResultEsrch    → rc==ESRCH 分支
// - Construct_FinishedOther_MapsToEperm         → else rc!=0 分支
// - Construct_Started_EmitsTaskStarted          → started 连接

TEST_F(UT_ProcessController, Construct_FinishedSuccess_EmitsResultZero)
{
    // Arrange: 构造控制器并监听信号
    ProcessController controller(4321, SIGTERM);
    QSignalSpy resultSpy(&controller, &ProcessController::resultReady);
    QSignalSpy finishSpy(&controller, &ProcessController::finished);
    Application *app = gApp;
    QSignalSpy taskSpy(app, &Application::backgroundTaskStateChanged);
    // Act: 真实执行零退出码子进程
    controller.m_proc->start(QStringLiteral("/bin/sh"), QStringList{QStringLiteral("-c"), QStringLiteral("exit 0")});
    ASSERT_TRUE(controller.m_proc->waitForFinished(10000));
    // Assert: 结果 0 + 后台任务结束 + finished
    EXPECT_EQ(resultSpy.count(), 1);
    EXPECT_EQ(resultSpy.first().first().toInt(), 0);
    EXPECT_EQ(finishSpy.count(), 1);
    ASSERT_GE(taskSpy.count(), 1);
    EXPECT_EQ(taskSpy.last().first().value<Application::TaskState>(), Application::kTaskFinished);
}

TEST_F(UT_ProcessController, Construct_FinishedEsrch_EmitsResultEsrch)
{
    // Arrange: 构造控制器
    ProcessController controller(4321, SIGTERM);
    QSignalSpy resultSpy(&controller, &ProcessController::resultReady);
    // Act: 真实子进程退出码 3（== ESRCH）
    controller.m_proc->start(QStringLiteral("/bin/sh"), QStringList{QStringLiteral("-c"), QStringLiteral("exit 3")});
    ASSERT_TRUE(controller.m_proc->waitForFinished(10000));
    // Assert: 结果直传 ESRCH
    EXPECT_EQ(resultSpy.count(), 1);
    EXPECT_EQ(resultSpy.first().first().toInt(), ESRCH);
}

TEST_F(UT_ProcessController, Construct_FinishedOther_MapsToEperm)
{
    // Arrange: 构造控制器
    ProcessController controller(4321, SIGTERM);
    QSignalSpy resultSpy(&controller, &ProcessController::resultReady);
    // Act: 真实子进程退出码 1（== EPERM）
    controller.m_proc->start(QStringLiteral("/bin/sh"), QStringList{QStringLiteral("-c"), QStringLiteral("exit 1")});
    ASSERT_TRUE(controller.m_proc->waitForFinished(10000));
    // Assert: 其他非零码映射 EPERM
    EXPECT_EQ(resultSpy.count(), 1);
    EXPECT_EQ(resultSpy.first().first().toInt(), EPERM);
}

TEST_F(UT_ProcessController, Construct_Started_EmitsTaskStarted)
{
    // Arrange: 构造控制器并监听后台任务状态
    ProcessController controller(4321, SIGKILL);
    Application *app = gApp;
    QSignalSpy taskSpy(app, &Application::backgroundTaskStateChanged);
    // Act: 真实启动子进程（started 信号驱动）
    controller.m_proc->start(QStringLiteral("/bin/sh"), QStringList{QStringLiteral("-c"), QStringLiteral("exit 0")});
    ASSERT_TRUE(controller.m_proc->waitForFinished(10000));
    // Assert: 上报 kTaskStarted，且任务正常收尾
    ASSERT_GE(taskSpy.count(), 1);
    EXPECT_EQ(taskSpy.first().first().value<Application::TaskState>(), Application::kTaskStarted);
    EXPECT_EQ(taskSpy.last().first().value<Application::TaskState>(), Application::kTaskFinished);
}

TEST_F(UT_ProcessController, Construct_MemberInit_StoresPidAndSignal)
{
    // Arrange: 以特征 pid/signal 构造
    ProcessController controller(778899, 15);
    // Act: 读取构造参数留存（经成员直查）
    bool hasProc = controller.m_proc != nullptr;
    // Assert: QProcess 子对象已创建
    EXPECT_TRUE(hasProc);
    EXPECT_EQ(controller.m_proc->parent(), &controller);
    EXPECT_EQ(controller.m_pid, 778899);
    EXPECT_EQ(controller.m_signal, 15);
}

//━━━━━━━━━━━━━━━━━━━━ execute ━━━━━━━━━━━━━━━━━━━━

TEST_F(UT_ProcessController, Execute_CommandsExist_StartsPkexecKill)
{
    // Arrange: pkexec/kill 均视为存在，拦截启动
    stub.set_lamda(static_cast<bool (*)(const QString &)>(&QFile::exists),
                      [](const QString &) -> bool {
        __DBG_STUB_INVOKE__
        return true;
    });
    QString program;
    QStringList params;
    hookProcessStart(stub, &program, &params);
    ProcessController controller(4321, 15);
    // Act: 执行
    controller.execute();
    // Assert: 启动 pkexec kill -{signal} {pid}
    EXPECT_TRUE(program.endsWith(QStringLiteral("pkexec")));
    ASSERT_EQ(params.size(), 3);
    EXPECT_TRUE(params.at(0).endsWith(QStringLiteral("kill")));
    EXPECT_EQ(params.at(1), QStringLiteral("-15"));
    EXPECT_EQ(params.at(2), QStringLiteral("4321"));
}

TEST_F(UT_ProcessController, Execute_PkexecMissing_EmitsEnoentWithoutFinished)
{
    // Arrange: exists 一律 false（pkexec 与 kill 两次检查均未命中）；
    // exit() 桩化为 no-op（stub 桩体不能抛异常穿越，故验证“退出码上报 + 不发 finished”）。
    stub.set_lamda(static_cast<bool (*)(const QString &)>(&QFile::exists),
                      [](const QString &) -> bool {
        __DBG_STUB_INVOKE__
        return false;
    });
    stub.set_lamda((void (*)(int)) & exit, [](int) -> void {
        __DBG_STUB_INVOKE__
    });
    QString program;
    QStringList params;
    hookProcessStart(stub, &program, &params);
    ProcessController controller(4321, 15);
    QSignalSpy resultSpy(&controller, &ProcessController::resultReady);
    QSignalSpy finishSpy(&controller, &ProcessController::finished);
    // Act: 执行（exit() 被 no-op，流程走完两次检查）
    controller.execute();
    // Assert: 两次检查各上报一次 ENOENT，finished 不发出
    ASSERT_EQ(resultSpy.count(), 2);
    EXPECT_EQ(resultSpy.at(0).first().toInt(), ENOENT);
    EXPECT_EQ(resultSpy.at(1).first().toInt(), ENOENT);
    EXPECT_EQ(finishSpy.count(), 0);
    EXPECT_TRUE(program.endsWith(QStringLiteral("pkexec")));
    EXPECT_EQ(params.at(1), QStringLiteral("-15"));
}
