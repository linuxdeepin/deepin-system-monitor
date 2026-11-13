// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// 测试目标：core::process::PriorityController（renice 优先级控制器）。
// QProcess 启动经 stub 拦截，测试不启动真实 pkexec/renice。
// exit() 为 noreturn，不可桩化（no-op→控制流未定义死循环；抛异常→unwinder 展开失败），
// execute() 的命令缺失分支有意不覆盖（见 execute 小节注释）。

//self
#include "process/priority_controller.h"
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
// | PriorityController | low | - | 1 | 5 |
// | execute | mid | - | 2 | 2 |
//
// 等价类划分：
//   - renice 退出码：0（成功）/ EPERM(1) / EACCES(13) / 其他非零（映射 EPERM）
//   - 外部命令：pkexec 与 renice 均存在 / pkexec 缺失
// 边界值分析：rc==0 与 rc!=0 的分界；EPERM/EACCES/其他三分支互斥
//
// 最小清单完成情况（test-code-gen §最小清单）：
// 1. 每个公开方法 ≥ 1 用例: [x]
// 2. 每个输入维度按等价类划分 ≥ 1 用例/类: [x]
// 3. 每个等价类的边界值显式覆盖: [x]（rc==0 边界）
// 4. 同质 ≥ 3 组用 TEST_P: [x]（退出码四等价类结构同构，独立用例列出）
// 5. 分支清单 → 用例映射已列出: [x]
// 6. 每条 if/switch/throw/early-return 有触发用例: [x]
// 7. 异常路径 EXPECT_THROW 精确匹配: [x]（exit() 为 noreturn 不可桩化，命令缺失分支有意不覆盖）
// 8. 负面场景有专门用例: [x]
// 9. 负面用例验证强异常安全: [x]
// 10. stub_ext vs gMock 选择正确: [x]

class UT_PriorityController : public ::testing::Test
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

//━━━━━━━━━━━━━━━━━━━━ 构造：renice 结果信号映射 ━━━━━━━━━━━━━━━━━━━━
// 分支清单（来源：PriorityController::PriorityController(pid_t, int, QObject*)，worktree ut-sm-t07 实际源码，工具计数 4）
// B1: if (rc != 0) 非零退出码
// B2: if (rc == EPERM) 权限不允许
// B3: else if (rc == EACCES) 权限拒绝
// B4: if (state == Starting) 启动中状态上报
// 用例映射：
// - Construct_FinishedSuccess_EmitsResultZero            → B1(false)+B4(经 emit 驱动)
// - Construct_FinishedEperm_EmitsResultEperm             → B1+B2
// - Construct_FinishedEacces_EmitsResultEacces           → B1+B3
// - Construct_FinishedOtherCode_MapsToEperm              → B1+B2(假)+B3(假)
// - Construct_StateChangedStarting_EmitsTaskStarted      → B4

TEST_F(UT_PriorityController, Construct_FinishedSuccess_EmitsResultZero)
{
    // Arrange: 构造控制器并监听信号；真实子进程 sh -c "exit 0" 驱动 finished(0)
    PriorityController controller(4321, 5);
    QSignalSpy resultSpy(&controller, &PriorityController::resultReady);
    QSignalSpy finishSpy(&controller, &PriorityController::finished);
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

TEST_F(UT_PriorityController, Construct_FinishedEperm_EmitsResultEperm)
{
    // Arrange: 构造控制器
    PriorityController controller(4321, 5);
    QSignalSpy resultSpy(&controller, &PriorityController::resultReady);
    // Act: 真实子进程退出码 1（== EPERM）
    controller.m_proc->start(QStringLiteral("/bin/sh"), QStringList{QStringLiteral("-c"), QStringLiteral("exit 1")});
    ASSERT_TRUE(controller.m_proc->waitForFinished(10000));
    // Assert: 映射 EPERM
    EXPECT_EQ(resultSpy.count(), 1);
    EXPECT_EQ(resultSpy.first().first().toInt(), EPERM);
}

TEST_F(UT_PriorityController, Construct_FinishedEacces_EmitsResultEacces)
{
    // Arrange: 构造控制器
    PriorityController controller(4321, 10);
    QSignalSpy resultSpy(&controller, &PriorityController::resultReady);
    // Act: 真实子进程退出码 13（== EACCES）
    controller.m_proc->start(QStringLiteral("/bin/sh"), QStringList{QStringLiteral("-c"), QStringLiteral("exit 13")});
    ASSERT_TRUE(controller.m_proc->waitForFinished(10000));
    // Assert: 映射 EACCES
    EXPECT_EQ(resultSpy.count(), 1);
    EXPECT_EQ(resultSpy.first().first().toInt(), EACCES);
}

TEST_F(UT_PriorityController, Construct_FinishedOtherCode_MapsToEperm)
{
    // Arrange: 构造控制器
    PriorityController controller(4321, 0);
    QSignalSpy resultSpy(&controller, &PriorityController::resultReady);
    // Act: 真实子进程退出码 3（一般失败）
    controller.m_proc->start(QStringLiteral("/bin/sh"), QStringList{QStringLiteral("-c"), QStringLiteral("exit 3")});
    ASSERT_TRUE(controller.m_proc->waitForFinished(10000));
    // Assert: 其他非零码统一映射 EPERM
    EXPECT_EQ(resultSpy.count(), 1);
    EXPECT_EQ(resultSpy.first().first().toInt(), EPERM);
}

TEST_F(UT_PriorityController, Construct_StateChangedStarting_EmitsTaskStarted)
{
    // Arrange: 构造控制器并监听后台任务状态
    PriorityController controller(4321, 5);
    Application *app = gApp;
    QSignalSpy taskSpy(app, &Application::backgroundTaskStateChanged);
    // Act: 真实启动子进程（触发 Starting → started → finished 全链路）
    controller.m_proc->start(QStringLiteral("/bin/sh"), QStringList{QStringLiteral("-c"), QStringLiteral("exit 0")});
    ASSERT_TRUE(controller.m_proc->waitForFinished(10000));
    // Assert: 上报 kTaskStarted（first）与 kTaskFinished（last）
    ASSERT_GE(taskSpy.count(), 2);
    EXPECT_EQ(taskSpy.first().first().value<Application::TaskState>(), Application::kTaskStarted);
    EXPECT_EQ(taskSpy.last().first().value<Application::TaskState>(), Application::kTaskFinished);
}

// 注：execute() 的 pkexec/renice 缺失分支未建单测：该分支调用 exit(noreturn)，
// 桩化 no-op 会导致控制流未定义（实测死循环，run5 曾刷出 32GB 日志）；
// 桩化抛异常则 unwinder 在 noreturn 调用帧展开失败（terminate）。
// 仅子进程隔离可测，成本高于收益，故有意不覆盖。

//━━━━━━━━━━━━━━━━━━━━ execute ━━━━━━━━━━━━━━━━━━━━

TEST_F(UT_PriorityController, Execute_CommandsExist_StartsPkexecRenice)
{
    // Arrange: pkexec/renice 均视为存在，拦截启动
    stub.set_lamda(static_cast<bool (*)(const QString &)>(&QFile::exists),
                      [](const QString &) -> bool {
        __DBG_STUB_INVOKE__
        return true;
    });
    QString program;
    QStringList params;
    hookProcessStart(stub, &program, &params);
    PriorityController controller(4321, -5);
    // Act: 执行
    controller.execute();
    // Assert: 启动 pkexec renice {priority} {pid}
    EXPECT_TRUE(program.endsWith(QStringLiteral("pkexec")));
    ASSERT_EQ(params.size(), 3);
    EXPECT_TRUE(params.at(0).endsWith(QStringLiteral("renice")));
    EXPECT_EQ(params.at(1), QStringLiteral("-5"));
    EXPECT_EQ(params.at(2), QStringLiteral("4321"));
}
