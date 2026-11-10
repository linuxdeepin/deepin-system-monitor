// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// qt-autotest-generator 生成：deepin-system-monitor-plugin-popup ProcessDB 单元测试
//
// 被测类：core::process::ProcessDB（deepin-system-monitor-plugin-popup/process/process_db.cpp）
// 测试策略：
//   - sendSignalToProcess（私有，经 -fno-access-control 直调）信号发送链路：
//     · kill(pid, sig) 打桩捕获 (pid, signal) 调用序列并控制返回值/errno
//     · SIGTERM/SIGKILL 先发 SIGCONT 的前置行为逐调用验证
//     · EPERM 提权路径：ProcessController::execute 打桩为直接 emit resultReady(code)，
//       绝不触发真实 pkexec（其内部 QFile 检查失败会 ::exit 杀死测试进程）
//   - QObject 信号用 QSignalSpy 捕获（popup Application 构造已注册 pid_t/ErrorContext 元类型）
//   - update() 的 WMWindowList/DesktopEntryCache/ProcessSet 依赖全部打桩为计数器
//
// 用例计数声明（self-check-structural 校验本块）：
// | method               | level | factors       | min | actual |
// |----------------------|-------|---------------|-----|--------|
// | sendSignalToProcess  | high  | complexity:21 | 4   | 11     |
// | update               | mid   | complexity:1  | 2   | 3      |
// | ~ProcessDB           | low   | complexity:3  | 1   | 1      |
// | instance             | low   | -             | 1   | 2      |
// | isCurrentProcess     | low   | -             | 1   | 1      |
// | endProcess           | low   | -             | 1   | 1      |
// | pauseProcess         | low   | -             | 1   | 1      |
// | processEuid          | low   | -             | 1   | 1      |
// | setProcessPriority   | low   | -             | 1   | 2      |

#include <gtest/gtest.h>
#include <stubext.h>

#include <QApplication>
#include <QSignalSpy>

#include <cerrno>
#include <csignal>
#include <utility>
#include <vector>

#include "process_db.h"
#include "process/desktop_entry_cache.h"
#include "process/process_controller.h"
#include "process/process_set.h"
#include "wm/wm_window_list.h"

using namespace core::process;
using namespace core::system;
using namespace core::wm;

// ---------------------------------------------------------------------------
// 分支清单（来源：ProcessDB::sendSignalToProcess）
// B1: signal == SIGTERM || SIGKILL → 先发 SIGCONT（防挂起）
// B2: SIGCONT kill 成功 → pkill(pid, signal)
// B3: SIGCONT kill 失败 EPERM → pctl 提权（不经 pkill）
// B4: SIGCONT kill 失败非 EPERM → errfmt + processControlResultReady + return
// B5: pkill kill 成功 → emitSignal(signal)
// B6: pkill kill 失败 EPERM → pctl 提权
// B7: pkill kill 失败非 EPERM → errfmt + processControlResultReady + return（不 emit 状态信号）
// B8: emitSignal SIGTERM → processEnded(pid)
// B9: emitSignal SIGSTOP → processPaused(pid, 'T')
// B10: emitSignal SIGCONT → processResumed(pid, 'R')
// B11: emitSignal SIGKILL → processKilled(pid)
// B12: emitSignal 其他信号 → 仅告警，无状态信号
// B13: pctl resultReady code != 0 → errfmt + processControlResultReady
// B14: pctl resultReady code == 0 → emitSignal(signal)
// B15: fmsg SIGTERM → 返回 "Failed to end process"
// B16: fmsg SIGSTOP → 返回 "Failed to pause process"
// B17: fmsg SIGCONT → 返回 "Failed to resume process"
// B18: fmsg SIGKILL → 返回 "Failed to kill process"
// B19: fmsg 其他信号 → 返回 "Unknown error"
// B20: B4 命中（SIGCONT 前置失败且非 EPERM）→ errfmt + processControlResultReady 后提前 return，不再 pkill
// B21: B7 命中（pkill 失败且非 EPERM）→ errfmt + processControlResultReady 后提前 return，不 emitSignal
// ---------------------------------------------------------------------------
// 分支清单（来源：ProcessDB::update）
// B1: m_desktopEntryTimeCount++ && >= DesktopEntryTimeCount → updateCache 并重置计数
// B2: 计数不足 → 跳过缓存更新
// B3: m_windowList->updateWindowListCache() 刷新窗口列表
// B4: m_procSet->refresh() 刷新进程集合
// ---------------------------------------------------------------------------
// 分支清单（来源：析构函数~ProcessDB，即 ProcessDB 的析构）
// B1: m_procSet 非空 → delete 并置空
// B2: m_windowList 非空 → delete 并置空
// B3: m_desktopEntryCache 非空 → delete 并置空
// ---------------------------------------------------------------------------

namespace {

constexpr pid_t kTestPid = 424242;

class ProcessDbTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_db = ProcessDB::instance();
        ASSERT_NE(m_db, nullptr);
    }

    void TearDown() override { stub.clear(); }

    // 打桩 kill() 并捕获 (pid, signal) 调用序列；failSig/failErr 控制失败注入
    struct KillCapture {
        std::vector<std::pair<pid_t, int>> calls;
        int failSig = 0;   // 该信号返回 -1 并置 errno（0 = 全部成功）
        int failErr = 0;
    };

    void stubKill(KillCapture &cap)
    {
        stub.set_lamda(::kill, [&cap](pid_t pid, int sig) -> int {
            cap.calls.push_back({pid, sig});
            if (cap.failSig == sig) {
                errno = cap.failErr;
                return -1;
            }
            return 0;
        });
    }

    // 打桩 ProcessController::execute：不发真实 pkexec，直接回放 resultReady(code)
    void stubController(int code)
    {
        stub.set_lamda(ADDR(ProcessController, execute),
                       [code](ProcessController *self) {
                           Q_EMIT self->resultReady(code);
                           Q_EMIT self->finished();
                       });
    }

    stub_ext::StubExt stub;
    ProcessDB *m_db = nullptr;
};

// ---------------------------------------------------------------------------
// instance / 访问器
// ---------------------------------------------------------------------------

TEST_F(ProcessDbTest, Instance_RepeatedCalls_ReturnsSameSingleton)
{
    // Arrange：单例首次创建（构造 ProcessSet/WMWindowList/DesktopEntryCache）
    // Act
    ProcessDB *a = ProcessDB::instance();
    ProcessDB *b = ProcessDB::instance();
    // Assert：同一实例且成员就绪
    EXPECT_EQ(a, b);
    EXPECT_NE(a->processSet(), nullptr);
    EXPECT_NE(a->windowList(), nullptr);
    EXPECT_NE(a->desktopEntryCache(), nullptr);
}

TEST_F(ProcessDbTest, ProcessEuid_AfterInit_MatchesRealEuid)
{
    // Arrange
    ProcessDB *db = ProcessDB::instance();
    // Act
    const uid_t euid = db->processEuid();
    // Assert
    EXPECT_EQ(geteuid(), euid);
    EXPECT_NE(nullptr, db->processSet());
}

TEST_F(ProcessDbTest, IsCurrentProcess_MatchesOwnPid_TrueElseFalse)
{
    // Arrange
    const pid_t self = getpid();
    // Act
    const bool own = ProcessDB::isCurrentProcess(self);
    // Assert
    EXPECT_TRUE(own);
    EXPECT_FALSE(ProcessDB::isCurrentProcess(kTestPid));
}

// ---------------------------------------------------------------------------
// ~ProcessDB
// ---------------------------------------------------------------------------

TEST_F(ProcessDbTest, Dtor_ScopeExit_DeletesMembers)
{
    // Arrange：栈对象持有 ProcessSet/WMWindowList/DesktopEntryCache
    // Act：作用域内成员可访问
    {
        ProcessDB localDb;
        EXPECT_NE(nullptr, localDb.processSet());
        EXPECT_NE(nullptr, localDb.windowList());
        EXPECT_NE(nullptr, localDb.desktopEntryCache());
    }
    // Assert：作用域退出触发 ~ProcessDB（三成员 delete），asan 兜底泄漏检查
    SUCCEED();
}

// ---------------------------------------------------------------------------
// update：计数分支 + 刷新链
// ---------------------------------------------------------------------------

TEST_F(ProcessDbTest, Update_FirstCall_TriggersDesktopCacheAndRefresh)
{
    // Arrange：desktopEntryTimeCount 初始为 DesktopEntryTimeCount(150) → 首次 update 必触发。
    // 单例跨用例共享 → 先显式归位初始值，保证用例独立可重复
    m_db->m_desktopEntryTimeCount = 150;
    int cacheUpdates = 0;
    int refreshes = 0;
    stub.set_lamda(ADDR(DesktopEntryCache, updateCache),
                   [&cacheUpdates](DesktopEntryCache *) { ++cacheUpdates; });
    stub.set_lamda(ADDR(WMWindowList, updateWindowListCache),
                   [](WMWindowList *) {});
    stub.set_lamda(ADDR(ProcessSet, refresh), [&refreshes](ProcessSet *) { ++refreshes; });
    // Act
    m_db->update();
    // Assert
    EXPECT_EQ(1, cacheUpdates);
    EXPECT_EQ(1, refreshes);
}

TEST_F(ProcessDbTest, Update_SecondCall_SkipsDesktopCacheRefresh)
{
    // Arrange：首次触发后计数归零，第二次不再触发（计数归位保证独立性）
    m_db->m_desktopEntryTimeCount = 150;
    int cacheUpdates = 0;
    int refreshes = 0;
    stub.set_lamda(ADDR(DesktopEntryCache, updateCache),
                   [&cacheUpdates](DesktopEntryCache *) { ++cacheUpdates; });
    stub.set_lamda(ADDR(WMWindowList, updateWindowListCache),
                   [](WMWindowList *) {});
    stub.set_lamda(ADDR(ProcessSet, refresh), [&refreshes](ProcessSet *) { ++refreshes; });
    // Act
    m_db->update();
    m_db->update();
    // Assert：缓存仅第一次触发，进程集合两次都刷新
    EXPECT_EQ(1, cacheUpdates);
    EXPECT_EQ(2, refreshes);
}

TEST_F(ProcessDbTest, Update_EveryCall_RefreshesProcessSet)
{
    // Arrange：连续触发窗口内多次 update（计数归位保证独立性）
    m_db->m_desktopEntryTimeCount = 150;
    int refreshes = 0;
    int cacheUpdates = 0;
    stub.set_lamda(ADDR(DesktopEntryCache, updateCache),
                   [&cacheUpdates](DesktopEntryCache *) { ++cacheUpdates; });
    stub.set_lamda(ADDR(WMWindowList, updateWindowListCache),
                   [](WMWindowList *) {});
    stub.set_lamda(ADDR(ProcessSet, refresh), [&refreshes](ProcessSet *) { ++refreshes; });
    // Act
    m_db->update();
    m_db->update();
    m_db->update();
    // Assert：每次 update 都刷新进程集合；缓存仅首次
    EXPECT_EQ(3, refreshes);
    EXPECT_EQ(1, cacheUpdates);
}

// ---------------------------------------------------------------------------
// sendSignalToProcess：直接成功路径（kill 全部返回 0）
// ---------------------------------------------------------------------------

TEST_F(ProcessDbTest, SendSignal_TermSuccess_SendsContThenTerm_EmitsProcessEnded)
{
    // Arrange
    KillCapture cap;
    stubKill(cap);
    QSignalSpy endedSpy(m_db, &ProcessDB::processEnded);
    QSignalSpy ctrlSpy(m_db, &ProcessDB::processControlResultReady);
    // Act
    m_db->sendSignalToProcess(kTestPid, SIGTERM);
    // Assert：先 SIGCONT 再 SIGTERM，正常 emit，无错误上报
    ASSERT_EQ(2, cap.calls.size());
    EXPECT_EQ(SIGCONT, cap.calls[0].second);
    EXPECT_EQ(SIGTERM, cap.calls[1].second);
    EXPECT_EQ(kTestPid, cap.calls[0].first);
    ASSERT_EQ(1, endedSpy.count());
    EXPECT_EQ(kTestPid, endedSpy.at(0).at(0).value<pid_t>());
    EXPECT_EQ(0, ctrlSpy.count());
}

TEST_F(ProcessDbTest, SendSignal_KillSuccess_EmitsProcessKilled)
{
    // Arrange
    KillCapture cap;
    stubKill(cap);
    QSignalSpy killedSpy(m_db, &ProcessDB::processKilled);
    QSignalSpy ctrlSpy(m_db, &ProcessDB::processControlResultReady);
    // Act
    m_db->sendSignalToProcess(kTestPid, SIGKILL);
    // Assert
    ASSERT_EQ(2, cap.calls.size());
    EXPECT_EQ(SIGCONT, cap.calls[0].second);
    EXPECT_EQ(SIGKILL, cap.calls[1].second);
    EXPECT_EQ(1, killedSpy.count());
    EXPECT_EQ(0, ctrlSpy.count());
}

TEST_F(ProcessDbTest, SendSignal_StopSkipsCont_EmitsProcessPausedWithStateT)
{
    // Arrange：SIGSTOP 无需先发 SIGCONT
    KillCapture cap;
    stubKill(cap);
    QSignalSpy pausedSpy(m_db, &ProcessDB::processPaused);
    QSignalSpy ctrlSpy(m_db, &ProcessDB::processControlResultReady);
    // Act
    m_db->sendSignalToProcess(kTestPid, SIGSTOP);
    // Assert：仅一次 kill(SIGSTOP)，paused(pid, 'T')
    ASSERT_EQ(1, cap.calls.size());
    EXPECT_EQ(SIGSTOP, cap.calls[0].second);
    ASSERT_EQ(1, pausedSpy.count());
    QList<QVariant> args = pausedSpy.takeFirst();
    EXPECT_EQ(kTestPid, args.at(0).value<pid_t>());
    EXPECT_EQ('T', args.at(1).toChar().toLatin1());
    EXPECT_EQ(0, ctrlSpy.count());
}

TEST_F(ProcessDbTest, SendSignal_ContSuccess_EmitsProcessResumedWithStateR)
{
    // Arrange
    KillCapture cap;
    stubKill(cap);
    QSignalSpy resumedSpy(m_db, &ProcessDB::processResumed);
    // Act
    m_db->sendSignalToProcess(kTestPid, SIGCONT);
    // Assert
    ASSERT_EQ(1, cap.calls.size());
    EXPECT_EQ(SIGCONT, cap.calls[0].second);
    ASSERT_EQ(1, resumedSpy.count());
    QList<QVariant> args = resumedSpy.takeFirst();
    EXPECT_EQ(kTestPid, args.at(0).value<pid_t>());
    EXPECT_EQ('R', args.at(1).toChar().toLatin1());
}

TEST_F(ProcessDbTest, SendSignal_UnexpectedSignal_EmitsNoStateSignal)
{
    // Arrange：SIGUSR1 不在 TERM/STOP/CONT/KILL 处理表内
    KillCapture cap;
    stubKill(cap);
    QSignalSpy endedSpy(m_db, &ProcessDB::processEnded);
    QSignalSpy pausedSpy(m_db, &ProcessDB::processPaused);
    QSignalSpy resumedSpy(m_db, &ProcessDB::processResumed);
    QSignalSpy killedSpy(m_db, &ProcessDB::processKilled);
    QSignalSpy ctrlSpy(m_db, &ProcessDB::processControlResultReady);
    // Act
    m_db->sendSignalToProcess(kTestPid, SIGUSR1);
    // Assert：kill 已发出，但无进程状态信号、无错误上报
    EXPECT_EQ(1, cap.calls.size());
    EXPECT_EQ(0, endedSpy.count());
    EXPECT_EQ(0, pausedSpy.count());
    EXPECT_EQ(0, resumedSpy.count());
    EXPECT_EQ(0, killedSpy.count());
    EXPECT_EQ(0, ctrlSpy.count());
}

// ---------------------------------------------------------------------------
// sendSignalToProcess：SIGCONT 前置失败（EPERM → 提权；其他 → 报错）
// ---------------------------------------------------------------------------

TEST_F(ProcessDbTest, SendSignal_ContEperm_ControllerSucceeds_EmitsProcessEnded)
{
    // Arrange：SIGCONT 被拒 → 直接进提权路径（不再 pkill），控制器成功。
    // 注意：单例 ProcessDB 的亲和线程是 SystemMonitorThread，production 内部
    // connect(resultReady→emitSignal) 走队列连接且该线程无事件循环 → 改用
    // 本线程（主线程）新建的局部 ProcessDB，使连接变为直连，信号可同步观测
    KillCapture cap;
    cap.failSig = SIGCONT;
    cap.failErr = EPERM;
    stubKill(cap);
    stubController(0);
    ProcessDB localDb;
    QSignalSpy endedSpy(&localDb, &ProcessDB::processEnded);
    QSignalSpy ctrlSpy(&localDb, &ProcessDB::processControlResultReady);
    // Act
    localDb.sendSignalToProcess(kTestPid, SIGTERM);
    // Assert：仅一次 kill(SIGCONT)，随后提权成功 → processEnded(pid)
    ASSERT_EQ(1, cap.calls.size());
    EXPECT_EQ(SIGCONT, cap.calls[0].second);
    ASSERT_EQ(1, endedSpy.count());
    EXPECT_EQ(kTestPid, endedSpy.at(0).at(0).value<pid_t>());
    EXPECT_EQ(0, ctrlSpy.count());
}

TEST_F(ProcessDbTest, SendSignal_ContEperm_ControllerFails_ReportsError)
{
    // Arrange
    KillCapture cap;
    cap.failSig = SIGCONT;
    cap.failErr = EPERM;
    stubKill(cap);
    stubController(1);
    ProcessDB localDb;   // 主线程局部实例：内部 connect 直连，信号可同步观测
    QSignalSpy endedSpy(&localDb, &ProcessDB::processEnded);
    QSignalSpy ctrlSpy(&localDb, &ProcessDB::processControlResultReady);
    // Act
    localDb.sendSignalToProcess(kTestPid, SIGTERM);
    // Assert：提权失败 → processControlResultReady 携带 kErrorTypeSystem
    EXPECT_EQ(0, endedSpy.count());
    ASSERT_EQ(1, ctrlSpy.count());
    ErrorContext ec = ctrlSpy.at(0).at(0).value<ErrorContext>();
    EXPECT_EQ(ErrorContext::kErrorTypeSystem, ec.getCode());
    EXPECT_NE(0, ec.getSubCode());
    EXPECT_FALSE(ec.getErrorMessage().isEmpty());
}

TEST_F(ProcessDbTest, SendSignal_ContFailsNonEperm_ReportsErrorWithoutPkill)
{
    // Arrange：SIGCONT 返回 ESRCH → 上报后直接返回，不发 SIGTERM
    KillCapture cap;
    cap.failSig = SIGCONT;
    cap.failErr = ESRCH;
    stubKill(cap);
    QSignalSpy endedSpy(m_db, &ProcessDB::processEnded);
    QSignalSpy ctrlSpy(m_db, &ProcessDB::processControlResultReady);
    // Act
    m_db->sendSignalToProcess(kTestPid, SIGTERM);
    // Assert：仅一次 kill，错误码 ESRCH，无 processEnded
    ASSERT_EQ(1, cap.calls.size());
    ASSERT_EQ(1, ctrlSpy.count());
    ErrorContext ec = ctrlSpy.at(0).at(0).value<ErrorContext>();
    EXPECT_EQ(ErrorContext::kErrorTypeSystem, ec.getCode());
    EXPECT_EQ(ESRCH, ec.getSubCode());
    EXPECT_EQ(0, endedSpy.count());
}

// ---------------------------------------------------------------------------
// sendSignalToProcess：pkill 阶段失败
// ---------------------------------------------------------------------------

TEST_F(ProcessDbTest, SendSignal_PkillEperm_Stop_UsesController)
{
    // Arrange：SIGSTOP kill 遭 EPERM → 提权成功 → paused
    KillCapture cap;
    cap.failSig = SIGSTOP;
    cap.failErr = EPERM;
    stubKill(cap);
    stubController(0);
    ProcessDB localDb;   // 主线程局部实例：内部 connect 直连，信号可同步观测
    QSignalSpy pausedSpy(&localDb, &ProcessDB::processPaused);
    QSignalSpy ctrlSpy(&localDb, &ProcessDB::processControlResultReady);
    // Act
    localDb.sendSignalToProcess(kTestPid, SIGSTOP);
    // Assert：kill(SIGSTOP) 遭 EPERM → 提权成功 → processPaused(pid, 'T')
    ASSERT_EQ(1, cap.calls.size());
    EXPECT_EQ(SIGSTOP, cap.calls[0].second);
    ASSERT_EQ(1, pausedSpy.count());
    EXPECT_EQ(kTestPid, pausedSpy.at(0).at(0).value<pid_t>());
    EXPECT_EQ(0, ctrlSpy.count());
}

TEST_F(ProcessDbTest, ResumeProcess_KillStubbed_DelegatesToContFlow)
{
    // Arrange：resumeProcess → sendSignalToProcess(pid, SIGCONT)（else 分支直接 pkill）
    KillCapture cap;
    stubKill(cap);
    QSignalSpy resumedSpy(m_db, &ProcessDB::processResumed);
    // Act
    m_db->resumeProcess(kTestPid);
    // Assert：一次 kill(SIGCONT) 成功 → processResumed(pid, 'R')
    ASSERT_EQ(1, cap.calls.size());
    EXPECT_EQ(SIGCONT, cap.calls[0].second);
    ASSERT_EQ(1, resumedSpy.count());
    EXPECT_EQ(kTestPid, resumedSpy.at(0).at(0).value<pid_t>());
    EXPECT_EQ('R', resumedSpy.at(0).at(1).toChar().toLatin1());
}

TEST_F(ProcessDbTest, KillProcess_DelegatesToKillFlow_EmitsKilled)
{
    // Arrange：killProcess → sendSignalToProcess(pid, SIGKILL)（TERM/KILL 分支，
    // 先发前置 SIGCONT 再 pkill SIGKILL）
    KillCapture cap;
    stubKill(cap);
    QSignalSpy killedSpy(m_db, &ProcessDB::processKilled);
    // Act
    m_db->killProcess(kTestPid);
    // Assert：调用序列 [SIGCONT, SIGKILL] → processKilled(pid)
    ASSERT_EQ(2, cap.calls.size());
    EXPECT_EQ(SIGCONT, cap.calls[0].second);
    EXPECT_EQ(SIGKILL, cap.calls[1].second);
    ASSERT_EQ(1, killedSpy.count());
    EXPECT_EQ(kTestPid, killedSpy.at(0).at(0).value<pid_t>());
}

TEST_F(ProcessDbTest, SendSignal_ContPkillFailsNonEperm_ReportsResumeError)
{
    // Arrange：真正的 SIGCONT 信号 pkill 阶段 ESRCH → fmsg(CONT) 臂 → 报错
    KillCapture cap;
    cap.failSig = SIGCONT;
    cap.failErr = ESRCH;
    stubKill(cap);
    QSignalSpy resumedSpy(m_db, &ProcessDB::processResumed);
    QSignalSpy ctrlSpy(m_db, &ProcessDB::processControlResultReady);
    // Act
    m_db->sendSignalToProcess(kTestPid, SIGCONT);
    // Assert：无 resumed，上报 kErrorTypeSystem/ESRCH
    EXPECT_EQ(0, resumedSpy.count());
    ASSERT_EQ(1, ctrlSpy.count());
    ErrorContext ec = ctrlSpy.at(0).at(0).value<ErrorContext>();
    EXPECT_EQ(ErrorContext::kErrorTypeSystem, ec.getCode());
    EXPECT_EQ(ESRCH, ec.getSubCode());
}

TEST_F(ProcessDbTest, SendSignal_KillEperm_ControllerFails_ReportsKillError)
{
    // Arrange：SIGKILL pkill 阶段 EPERM → 提权失败 → fmsg(KILL) 臂。
    // 用主线程局部 ProcessDB：内部 connect 直连，信号可同步观测
    KillCapture cap;
    cap.failSig = SIGKILL;
    cap.failErr = EPERM;
    stubKill(cap);
    stubController(1);
    ProcessDB localDb;
    QSignalSpy killedSpy(&localDb, &ProcessDB::processKilled);
    QSignalSpy ctrlSpy(&localDb, &ProcessDB::processControlResultReady);
    // Act
    localDb.killProcess(kTestPid);
    // Assert：调用序列 [SIGCONT, SIGKILL]，提权失败 → 报错、无 killed
    ASSERT_EQ(2, cap.calls.size());
    EXPECT_EQ(SIGKILL, cap.calls[1].second);
    EXPECT_EQ(0, killedSpy.count());
    ASSERT_EQ(1, ctrlSpy.count());
    ErrorContext ec = ctrlSpy.at(0).at(0).value<ErrorContext>();
    EXPECT_EQ(ErrorContext::kErrorTypeSystem, ec.getCode());
    EXPECT_NE(0, ec.getSubCode());
}

TEST_F(ProcessDbTest, SendSignal_PkillFailsNonEperm_Stop_ReportsErrorNoSignal)
{
    // Arrange：SIGSTOP kill 返回 EINVAL → 报错且不发状态信号
    KillCapture cap;
    cap.failSig = SIGSTOP;
    cap.failErr = EINVAL;
    stubKill(cap);
    QSignalSpy pausedSpy(m_db, &ProcessDB::processPaused);
    QSignalSpy ctrlSpy(m_db, &ProcessDB::processControlResultReady);
    // Act
    m_db->sendSignalToProcess(kTestPid, SIGSTOP);
    // Assert
    EXPECT_EQ(0, pausedSpy.count());
    ASSERT_EQ(1, ctrlSpy.count());
    ErrorContext ec = ctrlSpy.at(0).at(0).value<ErrorContext>();
    EXPECT_EQ(ErrorContext::kErrorTypeSystem, ec.getCode());
    EXPECT_EQ(EINVAL, ec.getSubCode());
    EXPECT_TRUE(ec.getErrorMessage().contains(QString::number(kTestPid)));
}

// ---------------------------------------------------------------------------
// 公共槽：endProcess / pauseProcess 委托 sendSignalToProcess
// ---------------------------------------------------------------------------

TEST_F(ProcessDbTest, EndProcess_KillStubbed_DelegatesToTermFlow)
{
    // Arrange
    KillCapture cap;
    stubKill(cap);
    QSignalSpy endedSpy(m_db, &ProcessDB::processEnded);
    // Act
    m_db->endProcess(kTestPid);
    // Assert：与 sendSignalToProcess(pid, SIGTERM) 等价
    ASSERT_EQ(2, cap.calls.size());
    EXPECT_EQ(SIGTERM, cap.calls[1].second);
    EXPECT_EQ(1, endedSpy.count());
}

TEST_F(ProcessDbTest, PauseProcess_KillStubbed_DelegatesToStopFlow)
{
    // Arrange
    KillCapture cap;
    stubKill(cap);
    QSignalSpy pausedSpy(m_db, &ProcessDB::processPaused);
    // Act
    m_db->pauseProcess(kTestPid);
    // Assert：SIGSTOP 无 SIGCONT 前置
    ASSERT_EQ(1, cap.calls.size());
    EXPECT_EQ(SIGSTOP, cap.calls[0].second);
    EXPECT_EQ(1, pausedSpy.count());
}

// ---------------------------------------------------------------------------
// setProcessPriority 槽：发信号
// ---------------------------------------------------------------------------

TEST_F(ProcessDbTest, SetProcessPrioritySlot_AnyPriority_EmitsPrioritysetChanged)
{
    // Arrange
    QSignalSpy prioSpy(m_db, &ProcessDB::signalProcessPrioritysetChanged);
    // Act
    m_db->setProcessPriority(kTestPid, 5);
    // Assert
    ASSERT_EQ(1, prioSpy.count());
    QList<QVariant> args = prioSpy.takeFirst();
    EXPECT_EQ(kTestPid, args.at(0).value<pid_t>());
    EXPECT_EQ(5, args.at(1).toInt());
}

TEST_F(ProcessDbTest, SetProcessPrioritySlot_NormalPriority_EmitsZero)
{
    // Arrange
    QSignalSpy prioSpy(m_db, &ProcessDB::signalProcessPrioritysetChanged);
    // Act
    m_db->setProcessPriority(kTestPid, 0);
    // Assert：信号携带 pid 与优先级 0
    ASSERT_EQ(1, prioSpy.count());
    EXPECT_EQ(kTestPid, prioSpy.at(0).at(0).value<pid_t>());
    EXPECT_EQ(0, prioSpy.at(0).at(1).toInt());
}

}   // namespace
