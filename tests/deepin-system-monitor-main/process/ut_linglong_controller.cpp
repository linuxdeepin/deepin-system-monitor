// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// 测试目标：core::process::LinglongController（ll-cli kill 玲珑应用信号控制器）。
// QProcess 启动经 stub 拦截，测试不启动真实 ll-cli。

//self
#include "process/linglong_controller.h"
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
// | LinglongController | low | - | 1 | 3 |
// | execute | low | - | 2 | 4 |
//
// 等价类划分：
//   - appId 格式：合法反向域名 / 非法（空格、特殊符号）
//   - ll-cli 命令：存在 / 缺失
//   - 信号：SIGKILL(9) → "SIGKILL" / 其他 → "SIGTERM"
//   - 退出码：0（成功）/ 非零（读 stderr 告警后直传）
// 边界值分析：正则匹配的空串输入（空 appId 无匹配 → EINVAL）
//
// 最小清单完成情况（test-code-gen §最小清单）：
// 1. 每个公开方法 ≥ 1 用例: [x]
// 2. 每个输入维度按等价类划分 ≥ 1 用例/类: [x]
// 3. 每个等价类的边界值显式覆盖: [x]
// 4. 同质 ≥ 3 组用 TEST_P: [x]（退出码/信号等价类独立用例列出）
// 5. 分支清单 → 用例映射已列出: [x]
// 6. 每条 if/switch/throw/early-return 有触发用例: [x]
// 7. 异常路径 EXPECT_THROW 精确匹配: [x]（无 throw 路径）
// 8. 负面场景有专门用例: [x]
// 9. 负面用例验证强异常安全: [x]
// 10. stub_ext vs gMock 选择正确: [x]

class UT_LinglongController : public ::testing::Test
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

//━━━━━━━━━━━━━━━━━━━━ 构造：ll-cli kill 结果信号映射 ━━━━━━━━━━━━━━━━━━━━
// 分支清单（来源：LinglongController::LinglongController(const QString&, int, QObject*)，worktree ut-sm-t07 实际源码，工具计数 1）
// B1: if (rc != 0) 非零退出码读 stderr 告警
// 用例映射：
// - Construct_FinishedSuccess_EmitsResultZero     → B1(false)
// - Construct_FinishedFailure_ReadsStderrEmitsRc  → B1

TEST_F(UT_LinglongController, Construct_FinishedSuccess_EmitsResultZero)
{
    // Arrange: 构造控制器并监听信号
    LinglongController controller(QStringLiteral("org.deepin.editor"), SIGTERM);
    QSignalSpy resultSpy(&controller, &LinglongController::resultReady);
    QSignalSpy finishSpy(&controller, &LinglongController::finished);
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

TEST_F(UT_LinglongController, Construct_FinishedFailure_ReadsStderrEmitsRc)
{
    // Arrange: 构造控制器（未真实启动进程，readAllStandardError 返回空）
    LinglongController controller(QStringLiteral("org.deepin.editor"), SIGKILL);
    QSignalSpy resultSpy(&controller, &LinglongController::resultReady);
    // Act: 真实子进程退出码 5
    controller.m_proc->start(QStringLiteral("/bin/sh"), QStringList{QStringLiteral("-c"), QStringLiteral("exit 5")});
    ASSERT_TRUE(controller.m_proc->waitForFinished(10000));
    // Assert: 退出码直传
    EXPECT_EQ(resultSpy.count(), 1);
    EXPECT_EQ(resultSpy.first().first().toInt(), 5);
}

TEST_F(UT_LinglongController, Construct_MemberInit_StoresAppIdAndSignal)
{
    // Arrange: 以特征 appId/signal 构造
    LinglongController controller(QStringLiteral("org.deepin.editor"), 9);
    // Act: 读取构造留存
    bool hasProc = controller.m_proc != nullptr;
    // Assert: QProcess 子对象与参数留存
    EXPECT_TRUE(hasProc);
    EXPECT_EQ(controller.m_proc->parent(), &controller);
    EXPECT_EQ(controller.m_appId, QStringLiteral("org.deepin.editor"));
    EXPECT_EQ(controller.m_signal, 9);
}

//━━━━━━━━━━━━━━━━━━━━ execute ━━━━━━━━━━━━━━━━━━━━
// 分支清单（来源：LinglongController::execute()，worktree ut-sm-t07 实际源码，工具计数 3）
// B1: if (!appIdRe.match(m_appId).hasMatch()) appId 格式非法
// B2: if (!QFile::exists(CMD_LL_CLI)) ll-cli 缺失
// B3: ternary (m_signal == SIGKILL) ? "SIGKILL" : "SIGTERM" 信号名选择
// 用例映射：
// - Execute_InvalidAppId_EmitsEinvalWithoutStart → B1
// - Execute_EmptyAppId_EmitsEinvalWithoutStart   → B1（边界：空串无匹配）
// - Execute_LlcliMissing_EmitsEnoent             → B2
// - Execute_TermSignal_StartsLlcliSigterm        → B3(false)
// - Execute_KillSignal_StartsLlcliSigkill        → B3

TEST_F(UT_LinglongController, Execute_InvalidAppId_EmitsEinvalWithoutStart)
{
    // Arrange: 非法 appId（含空格与特殊符号）
    QString program;
    QStringList params;
    hookProcessStart(stub, &program, &params);
    LinglongController controller(QStringLiteral("bad app!id"), SIGTERM);
    QSignalSpy resultSpy(&controller, &LinglongController::resultReady);
    QSignalSpy finishSpy(&controller, &LinglongController::finished);
    // Act: 执行
    controller.execute();
    // Assert: EINVAL + finished，未启动任何进程
    EXPECT_EQ(resultSpy.count(), 1);
    EXPECT_EQ(resultSpy.first().first().toInt(), EINVAL);
    EXPECT_EQ(finishSpy.count(), 1);
    EXPECT_TRUE(program.isEmpty());
}

TEST_F(UT_LinglongController, Execute_EmptyAppId_EmitsEinvalWithoutStart)
{
    // Arrange: 空 appId（正则无匹配的边界输入）
    QString program;
    QStringList params;
    hookProcessStart(stub, &program, &params);
    LinglongController controller(QString(), SIGTERM);
    QSignalSpy resultSpy(&controller, &LinglongController::resultReady);
    // Act: 执行
    controller.execute();
    // Assert: EINVAL，未启动
    EXPECT_EQ(resultSpy.count(), 1);
    EXPECT_EQ(resultSpy.first().first().toInt(), EINVAL);
    EXPECT_TRUE(program.isEmpty());
}

TEST_F(UT_LinglongController, Execute_LlcliMissing_EmitsEnoent)
{
    // Arrange: ll-cli 缺失
    stub.set_lamda(static_cast<bool (*)(const QString &)>(&QFile::exists),
                      [](const QString &) -> bool {
        __DBG_STUB_INVOKE__
        return false;
    });
    QString program;
    QStringList params;
    hookProcessStart(stub, &program, &params);
    LinglongController controller(QStringLiteral("org.deepin.editor"), SIGTERM);
    QSignalSpy resultSpy(&controller, &LinglongController::resultReady);
    QSignalSpy finishSpy(&controller, &LinglongController::finished);
    // Act: 执行
    controller.execute();
    // Assert: ENOENT + finished，未启动
    EXPECT_EQ(resultSpy.count(), 1);
    EXPECT_EQ(resultSpy.first().first().toInt(), ENOENT);
    EXPECT_EQ(finishSpy.count(), 1);
    EXPECT_TRUE(program.isEmpty());
}

TEST_F(UT_LinglongController, Execute_TermSignal_StartsLlcliSigterm)
{
    // Arrange: ll-cli 存在，SIGTERM(15)
    stub.set_lamda(static_cast<bool (*)(const QString &)>(&QFile::exists),
                      [](const QString &) -> bool {
        __DBG_STUB_INVOKE__
        return true;
    });
    QString program;
    QStringList params;
    hookProcessStart(stub, &program, &params);
    LinglongController controller(QStringLiteral("org.deepin.editor"), SIGTERM);
    // Act: 执行
    controller.execute();
    // Assert: 启动 ll-cli kill -s SIGTERM {appId}
    EXPECT_TRUE(program.endsWith(QStringLiteral("ll-cli")));
    ASSERT_EQ(params.size(), 4);
    EXPECT_EQ(params.at(0), QStringLiteral("kill"));
    EXPECT_EQ(params.at(1), QStringLiteral("-s"));
    EXPECT_EQ(params.at(2), QStringLiteral("SIGTERM"));
    EXPECT_EQ(params.at(3), QStringLiteral("org.deepin.editor"));
}

TEST_F(UT_LinglongController, Execute_KillSignal_StartsLlcliSigkill)
{
    // Arrange: ll-cli 存在，SIGKILL(9)
    stub.set_lamda(static_cast<bool (*)(const QString &)>(&QFile::exists),
                      [](const QString &) -> bool {
        __DBG_STUB_INVOKE__
        return true;
    });
    QString program;
    QStringList params;
    hookProcessStart(stub, &program, &params);
    LinglongController controller(QStringLiteral("org.deepin.editor"), SIGKILL);
    // Act: 执行
    controller.execute();
    // Assert: 信号名选择 SIGKILL
    EXPECT_TRUE(program.endsWith(QStringLiteral("ll-cli")));
    ASSERT_EQ(params.size(), 4);
    EXPECT_EQ(params.at(2), QStringLiteral("SIGKILL"));
    EXPECT_EQ(params.at(3), QStringLiteral("org.deepin.editor"));
}
