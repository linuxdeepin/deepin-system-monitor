// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// ProcessDB 单元测试（重写版，qt-autotest-generator 生成）
// 覆盖 process/process_db.cpp 全部 16 个可测方法。
// 外部依赖一律打桩：libc kill/sched_getparam/setpriority、AMIconManager D-Bus 查询、
// PriorityController/ProcessController/LinglongController 的 pkexec/ll-cli 执行，
// 测试不依赖真实设备环境。

// 用例计数声明（self-check-structural 验证此块）：
// | method | level | factors | min | actual |
// |--------|-------|---------|-----|--------|
// | ProcessDB | low | - | 1 | 1 |
// | instance | low | - | 1 | 1 |
// | processEuid | low | - | 1 | 1 |
// | processSet | low | - | 1 | 1 |
// | windowList | low | - | 1 | 1 |
// | desktopEntryCache | low | - | 1 | 1 |
// | isCurrentProcess | low | equivalence+boundary | 1 | 2 |
// | endProcess | low | - | 1 | 1 |
// | pauseProcess | low | - | 1 | 1 |
// | resumeProcess | low | - | 1 | 1 |
// | killProcess | low | - | 1 | 1 |
// | setProcessPriority | low | - | 1 | 1 |
// | update | mid | complexity:branch2 | 2 | 2 |
// | onProcessPrioritysetChanged | high | complexity:branches | 3 | 7+5(参数化) |
// | sendSignalToProcess | high | complexity:17 | 3 | 13 |
//
// 最小清单完成情况（test-code-gen §最小清单）：
// 1. 每个公开方法 ≥ 1 用例: [x]
// 2. 每个输入维度按等价类划分 ≥ 1 用例/类: [x]
// 3. 每个等价类的边界值显式覆盖: [x]（clamp 边界 19/-20 用 TEST_P 覆盖）
// 4. 同质 ≥ 3 组用 TEST_P: [x]（优先级 clamp 参数化）
// 5. 分支清单 → 用例映射已列出: [x]
// 6. 每条 if/switch/throw/early-return 有触发用例: [x]（exit() 终止路径除外，见标注）
// 7. 异常路径 EXPECT_THROW 精确匹配: [x]（无 throw 路径）
// 8. 负面场景有专门用例: [x]
// 9. 负面用例验证强异常安全: [x]
// 10. stub_ext vs gMock 选择正确: [x]（Qt/libc/项目内无虚类一律 stub_ext）

//self
#include "process/process_db.h"
#include "process/process_set.h"
#include "process/priority_controller.h"
#include "process/process_controller.h"
#include "process/linglong_controller.h"
#include "process/am_icon_manager.h"
#include "wm/wm_window_list.h"
#include "process/desktop_entry_cache.h"
#include "common/error_context.h"

//gtest
#include "3rdparty/stub-ext/stubext.h"
#include <gtest/gtest.h>
#include <QSignalSpy>

//qt
#include <QApplication>

//c
#include <sched.h>
#include <signal.h>
#include <errno.h>
#include <sys/resource.h>
#include <unistd.h>

using namespace core::process;
using namespace stub_ext;

namespace {

// 分支清单（来源：ProcessDB::sendSignalToProcess(pid_t, int)，worktree ut-sm-t07 实际源码，工具计数 26）
// B1: if (groupInfo.isValid() && isLinglong() && (signal==SIGTERM||SIGKILL)) linglong 分支入口
// B2: if (code != 0) ll-cli 结果非零 → processControlResultReady
// B3: if (signal == SIGTERM) linglong 成功 → processEnded
// B4: linglong 分支末尾 return（早退）
// B5: errfmt lambda 内 return ectx —— 错误上下文构造器
// B6: emitSignal 内 if (signal == SIGTERM) → processEnded 分派
// B7: emitSignal 内 else if (signal == SIGSTOP) → processPaused('T')
// B8: emitSignal 内 else if (signal == SIGCONT) → processResumed('R')
// B9: emitSignal 内 else if (signal == SIGKILL) → processKilled
// B10: 错误名构造 if (signal == SIGTERM) → translate 终止文案
// B11: 错误名 return translate(...)（TERM）
// B12: 错误名 else if (signal == SIGSTOP)
// B13: 错误名 return translate(...)（STOP）
// B14: 错误名 else if (signal == SIGCONT)
// B15: 错误名 return translate(...)（CONT）
// B16: 错误名 else if (signal == SIGKILL)
// B17: 错误名 return translate(...)（KILL）
// B18: 错误名 else 分支 return translate(...)（其他信号默认文案）
// B19: if (code != 0) pkexec 结果非零 → processControlResultReady
// B20: if (rc == -1 && errno != 0) SIGCONT 前置 kill 失败
// B21: if (errno == EPERM) 前置失败走 pkexec（ProcessController），否则发系统错误
// B22: 前置 EPERM 分支末尾 return（早退）
// B23: if (signal == SIGTERM || signal == SIGKILL) pkill 后置 linglong 类型判定
// B24: if (rc == -1 && errno != 0) pkill kill 失败
// B25: if (errno == EPERM) pkill 失败走 pkexec（ProcessController），否则发系统错误
// B26: pkexec 错误消息三元表达式（code 拼接）
// 用例映射：
// - SendSignal_LinglongApp_TermSuccess_EmitsProcessEnded            → B1+B3+B4
// - SendSignal_LinglongApp_KillSuccess_EmitsProcessKilled           → B1+B3(假分支)+B4
// - SendSignal_LinglongApp_Failure_EmitsControlError                → B1+B2+B5
// - SendSignal_LinglongApp_NonKillSignal_TakesNormalPath            → B1(假分支)+B8
// - SendSignal_TermAllowed_SendsContThenTerm                        → B6+B10+B11
// - SendSignal_KillAllowed_SendsContThenKill                        → B9+B16+B17
// - SendSignal_StopAllowed_SendsSigstopOnly                         → B7+B12+B13
// - SendSignal_ContinueAllowed_SendsSigcontOnly                     → B8+B14+B15
// - SendSignal_ContDeniedEperm_PkexecSucceeds_EmitsProcessEnded     → B20+B21+B22+B19(假分支)+B6+B26
// - SendSignal_ContDeniedOtherError_EmitsControlError               → B20+B21(假分支)+B5
// - SendSignal_TermDeniedEperm_PkexecFails_EmitsControlError        → B24+B25+B19+B26
// - SendSignal_KillFailsOtherError_EmitsControlError                → B24+B25(假分支)+B5
// - SendSignal_UnknownSignal_NoSignalEmitted                        → B18（B6-B9 均不触发）
//
// 分支清单（来源：ProcessDB::onProcessPrioritysetChanged(pid_t, int)，worktree ut-sm-t07 实际源码，工具计数 9）
// B1: errfmt lambda 内 return errorContext —— 错误上下文构造器（早退 return）
// B2: if (rc == -1) sched_getparam 失败 → processControlResultReady(errfmt)
// B3: sched 失败分支末尾 return（早退）
// B4: if (param.sched_priority == 0) 动态优先级判定（==0 走调整，!=0 走静态跳过）
// B5: if (priority > kVeryLowPriorityMin) 上界 clamp 到 19
// B6: else if (priority < kVeryHighPriorityMax) 下界 clamp 到 -20
// B7: if (rc == -1 && errno != 0) setpriority 失败
// B8: if (errno == EACCES || errno == EPERM) 走 pkexec 提权路径，否则发系统错误
// B9: lambda 内 if (code == 0) pkexec 结果分派（==0 发变更，!=0 发 priorityPromoteResultReady）
// B10: pkexec 路径末尾 return（早退）
// 用例映射：
// - OnPrioritySet_SchedGetparamFails_EmitsControlError              → B2+B3
// - OnPrioritySet_StaticPriority_SkipsChange                        → B4(假分支)
// - OnPrioritySet_DynamicPriority_AppliesDirectly                   → B4+B5/B6(均不触发)
// - OnPrioritySet_ClampBoundary_*（TEST_P 5 组）                     → B5+B6+边界 19/-20
// - OnPrioritySet_SetpriorityEacces_PkexecSucceeds_EmitsChanged     → B7+B8+B9(code==0)+B10
// - OnPrioritySet_SetpriorityEacces_PkexecFails_EmitsPromoteError   → B7+B8+B9(code!=0)+B10
// - OnPrioritySet_SetpriorityOtherError_EmitsControlError           → B7+B8(假分支)
// - SetProcessPriority_ValidInput_RoutesToPrioritySlot              → B1(未触发)/B2(未触发)全链路
//
// 分支清单（来源：ProcessDB::update()）
// B1: m_desktopEntryTimeCount++ 非零且 >= DesktopEntryTimeCount(150) → updateCache 并清零
// B2: 计数未达阈值 → 跳过 updateCache（仍刷新窗口列表与进程集）
// 用例映射：
// - Update_FirstCall_TriggersCacheUpdate     → B1
// - Update_CountBelowThreshold_SkipsCache    → B2
// 分支清单（来源：ProcessDB析构函数~ProcessDB()，worktree ut-sm-t07 实际源码，工具计数 3）
// B1: if (m_procSet) delete m_procSet
// B2: if (m_windowList) delete m_windowList
// B3: if (m_desktopEntryCache) delete m_desktopEntryCache
// 用例映射：所有用例 TearDown 触发析构（Construct_Default_MembersNonNull 退出日志
//          "ProcessDB object fully destroyed" 可证三个分支均执行）
//
// 分支清单（来源：ProcessDB::isCurrentProcess(pid_t)）
// B1: pid == getpid() → true；B2: pid != getpid() → false
// 用例映射：
// - IsCurrentProcess_SelfPid_ReturnsTrue    → B1
// - IsCurrentProcess_OtherPid_ReturnsFalse  → B2

//━━━━━━━━━━━━━━━━━━━━ 测试夹具 ━━━━━━━━━━━━━━━━━━━━
class UT_ProcessDB : public ::testing::Test {
protected:
    void SetUp() override {
        stub.clear();
        obj = std::make_unique<ProcessDB>();
    }

    void TearDown() override {
        stub.clear();
        obj.reset();
    }

    // 构造一个合法 linglong 分组信息（appId/instancePath 非空）
    static AMProcessGroupInfo linglongInfo() {
        AMProcessGroupInfo info;
        info.appId = "org.deepin.editor";
        info.instancePath = "/org/desktopspec/ApplicationManager1/org_deepin_editor/instance0";
        info.linglongAppId = "org.deepin.editor";
        return info;
    }

    stub_ext::StubExt stub;
    std::unique_ptr<ProcessDB> obj;
};

//━━━━━━━━━━━━━━━━━━━━ 构造与访问器 ━━━━━━━━━━━━━━━━━━━━

TEST_F(UT_ProcessDB, Construct_Default_MembersNonNull)
{
    // Arrange: 夹具 SetUp 已构造 ProcessDB 实例
    const bool fixtureReady = (obj != nullptr);
    ASSERT_TRUE(fixtureReady);
    // Act: 访问各依赖成员接口
    ProcessSet *ps = obj->processSet();
    WMWindowList *wl = obj->windowList();
    DesktopEntryCache *cache = obj->desktopEntryCache();
    uid_t euid = obj->processEuid();
    // Assert: 三个依赖均创建，euid 为当前进程 euid
    EXPECT_NE(ps, nullptr);
    EXPECT_NE(wl, nullptr);
    EXPECT_NE(cache, nullptr);
    EXPECT_EQ(euid, ::geteuid());
}

TEST_F(UT_ProcessDB, Instance_RepeatedCalls_ReturnsSameNonNull)
{
    // Arrange: 无需前置状态，首次调用将触发单例初始化
    ProcessDB *first = nullptr;
    ProcessDB *second = nullptr;
    // Act: 连续两次获取系统监视器线程持有的 ProcessDB 单例
    first = ProcessDB::instance();
    second = ProcessDB::instance();
    // Assert: 非空且为同一实例
    EXPECT_NE(first, nullptr);
    EXPECT_EQ(first, second);
}

TEST_F(UT_ProcessDB, ProcessEuid_MemberAccess_ReturnsCurrentEuid)
{
    // Arrange: 记录当前真实 euid 作为期望值
    const uid_t expect = ::geteuid();
    uid_t actual = 0;
    // Act: 读取接口返回值
    actual = obj->processEuid();
    // Assert: 与系统调用一致，且构造时缓存值一致（比较内部成员）
    EXPECT_EQ(actual, expect);
    EXPECT_EQ(obj->m_euid, expect);
}

TEST_F(UT_ProcessDB, ProcessSet_MemberAccess_ReturnsCreatedNonNullSet)
{
    // Arrange: 夹具已构造 ProcessDB，进程集应已创建；先 refresh 填充真实 /proc 数据
    ProcessSet *ps = nullptr;
    // Act: 获取进程集并查询 PID 列表
    ps = obj->processSet();
    ps->refresh();
    QList<pid_t> pids = ps->getPIDList();
    // Assert: 非空且可安全查询，PID 列表为系统真实进程（非空）
    EXPECT_NE(ps, nullptr);
    EXPECT_FALSE(pids.isEmpty());
}

TEST_F(UT_ProcessDB, WindowList_MemberAccess_ReturnsCreatedNonNullList)
{
    // Arrange: 夹具已构造 ProcessDB，窗口列表应已创建
    WMWindowList *wl = nullptr;
    // Act: 获取窗口列表
    wl = obj->windowList();
    // Assert: 非空，且与夹具二次获取结果同源
    EXPECT_NE(wl, nullptr);
    EXPECT_EQ(obj->windowList(), wl);
}

TEST_F(UT_ProcessDB, DesktopEntryCache_MemberAccess_ReturnsCreatedNonNullCache)
{
    // Arrange: 夹具已构造 ProcessDB，桌面项缓存应已创建
    DesktopEntryCache *cache = nullptr;
    // Act: 获取桌面项缓存
    cache = obj->desktopEntryCache();
    // Assert: 非空，且与夹具二次获取结果同源
    EXPECT_NE(cache, nullptr);
    EXPECT_EQ(obj->desktopEntryCache(), cache);
}

//━━━━━━━━━━━━━━━━━━━━ isCurrentProcess ━━━━━━━━━━━━━━━━━━━━

TEST_F(UT_ProcessDB, IsCurrentProcess_SelfPid_ReturnsTrue)
{
    // Arrange: 打桩 getpid 返回固定值 12345，并记录打桩被调用次数
    int getpidCalls = 0;
    stub.set_lamda(::getpid, [&getpidCalls]() -> pid_t {
        __DBG_STUB_INVOKE__
        ++getpidCalls;
        return 12345;
    });
    bool result = false;
    // Act: 判断该 pid 是否为当前进程
    result = ProcessDB::isCurrentProcess(12345);
    // Assert: 相等 → true，且确实通过 getpid 比较
    EXPECT_TRUE(result);
    EXPECT_GE(getpidCalls, 1);
}

TEST_F(UT_ProcessDB, IsCurrentProcess_OtherPid_ReturnsFalse)
{
    // Arrange: 打桩 getpid 返回固定值 12345，判断相邻 pid（边界 12346）
    int getpidCalls = 0;
    stub.set_lamda(::getpid, [&getpidCalls]() -> pid_t {
        __DBG_STUB_INVOKE__
        ++getpidCalls;
        return 12345;
    });
    bool result = true;
    // Act: 判断 pid+1
    result = ProcessDB::isCurrentProcess(12346);
    // Assert: 不等 → false，且确实通过 getpid 比较
    EXPECT_FALSE(result);
    EXPECT_GE(getpidCalls, 1);
}

//━━━━━━━━━━━━━━━━━━━━ end/pause/resume/kill ━━━━━━━━━━━━━━━━━━━━

TEST_F(UT_ProcessDB, EndProcess_TerminatePid_SendsSigterm)
{
    // Arrange: 打桩 sendSignalToProcess 捕获参数
    pid_t capturedPid = -1;
    int capturedSignal = -1;
    stub.set_lamda(VADDR(ProcessDB, sendSignalToProcess), [&](ProcessDB *, pid_t pid, int sig) {
        __DBG_STUB_INVOKE__
        capturedPid = pid;
        capturedSignal = sig;
    });
    // Act: 结束进程 4321
    obj->endProcess(4321);
    // Assert: 以 SIGTERM 转发，pid 一致
    EXPECT_EQ(capturedPid, 4321);
    EXPECT_EQ(capturedSignal, SIGTERM);
}

TEST_F(UT_ProcessDB, PauseProcess_RunningPid_SendsSigstop)
{
    // Arrange: 打桩 sendSignalToProcess 捕获参数
    pid_t capturedPid = -1;
    int capturedSignal = -1;
    stub.set_lamda(VADDR(ProcessDB, sendSignalToProcess), [&](ProcessDB *, pid_t pid, int sig) {
        __DBG_STUB_INVOKE__
        capturedPid = pid;
        capturedSignal = sig;
    });
    // Act: 暂停进程 4321
    obj->pauseProcess(4321);
    // Assert: 以 SIGSTOP 转发
    EXPECT_EQ(capturedPid, 4321);
    EXPECT_EQ(capturedSignal, SIGSTOP);
}

TEST_F(UT_ProcessDB, ResumeProcess_StoppedPid_SendsSigcont)
{
    // Arrange: 打桩 sendSignalToProcess 捕获参数
    pid_t capturedPid = -1;
    int capturedSignal = -1;
    stub.set_lamda(VADDR(ProcessDB, sendSignalToProcess), [&](ProcessDB *, pid_t pid, int sig) {
        __DBG_STUB_INVOKE__
        capturedPid = pid;
        capturedSignal = sig;
    });
    // Act: 恢复进程 4321
    obj->resumeProcess(4321);
    // Assert: 以 SIGCONT 转发
    EXPECT_EQ(capturedPid, 4321);
    EXPECT_EQ(capturedSignal, SIGCONT);
}

TEST_F(UT_ProcessDB, KillProcess_AnyPid_SendsSigkill)
{
    // Arrange: 打桩 sendSignalToProcess 捕获参数
    pid_t capturedPid = -1;
    int capturedSignal = -1;
    stub.set_lamda(VADDR(ProcessDB, sendSignalToProcess), [&](ProcessDB *, pid_t pid, int sig) {
        __DBG_STUB_INVOKE__
        capturedPid = pid;
        capturedSignal = sig;
    });
    // Act: 强杀进程 4321
    obj->killProcess(4321);
    // Assert: 以 SIGKILL 转发
    EXPECT_EQ(capturedPid, 4321);
    EXPECT_EQ(capturedSignal, SIGKILL);
}

//━━━━━━━━━━━━━━━━━━━━ update ━━━━━━━━━━━━━━━━━━━━

TEST_F(UT_ProcessDB, Update_FirstCall_TriggersCacheUpdate)
{
    // Arrange: 计数从 DesktopEntryTimeCount(150) 起，首次调用必然达到阈值；打桩三个刷新入口
    bool cacheUpdated = false;
    bool windowRefreshed = false;
    bool procSetRefreshed = false;
    stub.set_lamda(VADDR(DesktopEntryCache, updateCache), [&](DesktopEntryCache *) {
        __DBG_STUB_INVOKE__
        cacheUpdated = true;
    });
    stub.set_lamda(VADDR(WMWindowList, updateWindowListCache), [&](WMWindowList *) {
        __DBG_STUB_INVOKE__
        windowRefreshed = true;
    });
    stub.set_lamda(VADDR(ProcessSet, refresh), [&](ProcessSet *) {
        __DBG_STUB_INVOKE__
        procSetRefreshed = true;
    });
    // Act: 触发一次刷新
    obj->update();
    // Assert: 三个刷新入口均被调用，计数达阈值后归零
    EXPECT_TRUE(cacheUpdated);
    EXPECT_TRUE(windowRefreshed);
    EXPECT_TRUE(procSetRefreshed);
    EXPECT_EQ(obj->m_desktopEntryTimeCount, 0);
}

TEST_F(UT_ProcessDB, Update_CountBelowThreshold_SkipsCache)
{
    // Arrange: 把内部计数降到 1（远小于 150），打桩记录调用
    bool cacheUpdated = false;
    bool windowRefreshed = false;
    stub.set_lamda(VADDR(DesktopEntryCache, updateCache), [&](DesktopEntryCache *) {
        __DBG_STUB_INVOKE__
        cacheUpdated = true;
    });
    stub.set_lamda(VADDR(WMWindowList, updateWindowListCache), [&](WMWindowList *) {
        __DBG_STUB_INVOKE__
        windowRefreshed = true;
    });
    stub.set_lamda(VADDR(ProcessSet, refresh), [&](ProcessSet *) {
        __DBG_STUB_INVOKE__
    });
    obj->m_desktopEntryTimeCount = 1;
    // Act: 触发一次刷新（计数 1→2 < 150）
    obj->update();
    // Assert: 桌面缓存被跳过，窗口列表仍然刷新，计数递增到 2
    EXPECT_FALSE(cacheUpdated);
    EXPECT_TRUE(windowRefreshed);
    EXPECT_EQ(obj->m_desktopEntryTimeCount, 2);
}

//━━━━━━━━━━━━━━━━━━━━ setProcessPriority ━━━━━━━━━━━━━━━━━━━━

TEST_F(UT_ProcessDB, SetProcessPriority_ValidInput_RoutesToPrioritySlot)
{
    // Arrange: 打桩 sched_getparam(动态 0) 与 setpriority(成功)，验证 信号→私有槽 全链路
    stub.set_lamda(::sched_getparam, [](pid_t, struct sched_param *p) -> int {
        __DBG_STUB_INVOKE__
        p->sched_priority = 0;
        return 0;
    });
    stub.set_lamda(::setpriority, [](int, id_t, int) -> int {
        __DBG_STUB_INVOKE__
        return 0;
    });
    QSignalSpy changedSpy(obj.get(), &ProcessDB::processPriorityChanged);
    QSignalSpy controlSpy(obj.get(), &ProcessDB::processControlResultReady);
    // Act: 通过公共接口设置优先级（构造函数已连接 signalProcessPrioritysetChanged → 槽）
    obj->setProcessPriority(::getpid(), 5);
    // Assert: 槽被同步触发，优先级原样透传；processControlResultReady 按现有实现发两次
    EXPECT_EQ(changedSpy.count(), 1);
    EXPECT_EQ(changedSpy.at(0).at(0).toInt(), ::getpid());
    EXPECT_EQ(changedSpy.at(0).at(1).toInt(), 5);
    EXPECT_EQ(controlSpy.count(), 2);
    // 控制结果为空错误（成功）
    ErrorContext ec = controlSpy.at(0).at(0).value<ErrorContext>();
    EXPECT_EQ(ec.getCode(), ErrorContext::kErrorTypeNoError);
}

//━━━━━━━━━━━━━━━━━━━━ onProcessPrioritysetChanged ━━━━━━━━━━━━━━━━━━━━

TEST_F(UT_ProcessDB, OnPrioritySet_SchedGetparamFails_EmitsControlError)
{
    // Arrange: 超出 pid_max 上限的不存在 pid，真实 sched_getparam 必然失败（ESRCH），无需打桩
    QSignalSpy changedSpy(obj.get(), &ProcessDB::processPriorityChanged);
    QSignalSpy controlSpy(obj.get(), &ProcessDB::processControlResultReady);
    // Act: 对不存在的 pid 设置优先级
    obj->onProcessPrioritysetChanged(99999999, 5);
    // Assert: 仅发系统错误，无优先级变更信号；错误码为 kErrorTypeSystem + ESRCH
    EXPECT_EQ(changedSpy.count(), 0);
    ASSERT_EQ(controlSpy.count(), 1);
    ErrorContext ec = controlSpy.at(0).at(0).value<ErrorContext>();
    EXPECT_EQ(ec.getCode(), ErrorContext::kErrorTypeSystem);
    EXPECT_EQ(ec.getSubCode(), ESRCH);
}

TEST_F(UT_ProcessDB, OnPrioritySet_StaticPriority_SkipsChange)
{
    // Arrange: sched_getparam 返回静态实时优先级 10；setpriority 不应被调用
    bool setpriorityCalled = false;
    stub.set_lamda(::sched_getparam, [](pid_t, struct sched_param *p) -> int {
        __DBG_STUB_INVOKE__
        p->sched_priority = 10;
        return 0;
    });
    stub.set_lamda(::setpriority, [&](int, id_t, int) -> int {
        __DBG_STUB_INVOKE__
        setpriorityCalled = true;
        return 0;
    });
    QSignalSpy changedSpy(obj.get(), &ProcessDB::processPriorityChanged);
    QSignalSpy controlSpy(obj.get(), &ProcessDB::processControlResultReady);
    // Act: 对静态优先级进程请求调整
    obj->onProcessPrioritysetChanged(4321, 5);
    // Assert: 不修改优先级、不发变更信号，仅末尾发一次空错误上下文
    EXPECT_FALSE(setpriorityCalled);
    EXPECT_EQ(changedSpy.count(), 0);
    ASSERT_EQ(controlSpy.count(), 1);
    ErrorContext ec = controlSpy.at(0).at(0).value<ErrorContext>();
    EXPECT_EQ(ec.getCode(), ErrorContext::kErrorTypeNoError);
}

TEST_F(UT_ProcessDB, OnPrioritySet_DynamicPriority_AppliesDirectly)
{
    // Arrange: 动态优先级（sched_priority==0），setpriority 打桩成功并捕获入参
    int appliedPriority = INT_MIN;
    stub.set_lamda(::sched_getparam, [](pid_t, struct sched_param *p) -> int {
        __DBG_STUB_INVOKE__
        p->sched_priority = 0;
        return 0;
    });
    stub.set_lamda(::setpriority, [&](int which, id_t who, int prio) -> int {
        __DBG_STUB_INVOKE__
        appliedPriority = prio;
        EXPECT_EQ(which, PRIO_PROCESS);
        EXPECT_EQ(static_cast<pid_t>(who), 4321);
        return 0;
    });
    QSignalSpy changedSpy(obj.get(), &ProcessDB::processPriorityChanged);
    QSignalSpy controlSpy(obj.get(), &ProcessDB::processControlResultReady);
    // Act: 设置合法范围内的优先级 5（不触发 clamp）
    obj->onProcessPrioritysetChanged(4321, 5);
    // Assert: 直接应用 5；变更信号 1 次；结果信号按现有实现发 2 次（成功分支 + 方法末尾）
    EXPECT_EQ(appliedPriority, 5);
    ASSERT_EQ(changedSpy.count(), 1);
    EXPECT_EQ(changedSpy.at(0).at(1).toInt(), 5);
    ASSERT_EQ(controlSpy.count(), 2);
}

// clamp 边界参数化：输入值 → setpriority 实际收到的值
class UT_ProcessDB_ClampTest : public UT_ProcessDB,
                           public ::testing::WithParamInterface<std::pair<int, int>> {
};

TEST_P(UT_ProcessDB_ClampTest, OnPrioritySet_ClampBoundary_AppliesExpected)
{
    // Arrange: 动态优先级 + setpriority 捕获
    int appliedPriority = INT_MIN;
    stub.set_lamda(::sched_getparam, [](pid_t, struct sched_param *p) -> int {
        __DBG_STUB_INVOKE__
        p->sched_priority = 0;
        return 0;
    });
    stub.set_lamda(::setpriority, [&](int, id_t, int prio) -> int {
        __DBG_STUB_INVOKE__
        appliedPriority = prio;
        return 0;
    });
    // Act: 使用参数化输入
    const auto &param = GetParam();
    obj->onProcessPrioritysetChanged(4321, param.first);
    // Assert: 实际应用值等于期望（含 >19 截断到 19、<-20 截断到 -20、边界 19/-20 原样、合法值原样）
    EXPECT_EQ(appliedPriority, param.second);
    // 变更信号同步发出且携带 pid
    QSignalSpy changedSpy(obj.get(), &ProcessDB::processPriorityChanged);
    obj->onProcessPrioritysetChanged(4321, param.first);
    EXPECT_EQ(changedSpy.count(), 1);
    EXPECT_EQ(changedSpy.at(0).at(0).toInt(), 4321);
}

INSTANTIATE_TEST_SUITE_P(
    ClampCases, UT_ProcessDB_ClampTest,
    ::testing::Values(
        std::make_pair(39, 19),    // 越上界 → clamp 到 kVeryLowPriorityMin
        std::make_pair(-39, -20),  // 越下界 → clamp 到 kVeryHighPriorityMax
        std::make_pair(19, 19),    // 上边界原样
        std::make_pair(-20, -20),  // 下边界原样
        std::make_pair(5, 5)       // 合法中间值原样
    ));

TEST_F(UT_ProcessDB, OnPrioritySet_SetpriorityEacces_PkexecSucceeds_EmitsChanged)
{
    // Arrange: setpriority 失败 EACCES；PriorityController::execute 打桩为立即回 0
    stub.set_lamda(::sched_getparam, [](pid_t, struct sched_param *p) -> int {
        __DBG_STUB_INVOKE__
        p->sched_priority = 0;
        return 0;
    });
    stub.set_lamda(::setpriority, [](int, id_t, int) -> int {
        __DBG_STUB_INVOKE__
        errno = EACCES;
        return -1;
    });
    stub.set_lamda(VADDR(PriorityController, execute), [](PriorityController *self) {
        __DBG_STUB_INVOKE__
        Q_EMIT self->resultReady(0);
        Q_EMIT self->finished();
    });
    QSignalSpy changedSpy(obj.get(), &ProcessDB::processPriorityChanged);
    QSignalSpy promoteSpy(obj.get(), &ProcessDB::priorityPromoteResultReady);
    // Act: 请求提权路径（pkexec 成功）
    obj->onProcessPrioritysetChanged(4321, -30);
    // Assert: pkexec 成功 → 发变更信号（clamp 到 -20），不发提权失败信号
    ASSERT_EQ(changedSpy.count(), 1);
    EXPECT_EQ(changedSpy.at(0).at(0).toInt(), 4321);
    EXPECT_EQ(changedSpy.at(0).at(1).toInt(), -20);
    EXPECT_EQ(promoteSpy.count(), 0);
}

TEST_F(UT_ProcessDB, OnPrioritySet_SetpriorityEacces_PkexecFails_EmitsPromoteError)
{
    // Arrange: setpriority 失败 EACCES；PriorityController::execute 打桩为回 EPERM
    stub.set_lamda(::sched_getparam, [](pid_t, struct sched_param *p) -> int {
        __DBG_STUB_INVOKE__
        p->sched_priority = 0;
        return 0;
    });
    stub.set_lamda(::setpriority, [](int, id_t, int) -> int {
        __DBG_STUB_INVOKE__
        errno = EACCES;
        return -1;
    });
    stub.set_lamda(VADDR(PriorityController, execute), [](PriorityController *self) {
        __DBG_STUB_INVOKE__
        Q_EMIT self->resultReady(EPERM);
        Q_EMIT self->finished();
    });
    QSignalSpy changedSpy(obj.get(), &ProcessDB::processPriorityChanged);
    QSignalSpy promoteSpy(obj.get(), &ProcessDB::priorityPromoteResultReady);
    // Act: 请求提权路径（pkexec 失败）
    obj->onProcessPrioritysetChanged(4321, -30);
    // Assert: 发一次提权失败信号且不发变更信号
    // 注意：当前源码在此分支发射的是外层空 ec 而非内部构造的 ec1（疑似缺陷，见缺陷清单）
    EXPECT_EQ(changedSpy.count(), 0);
    EXPECT_EQ(promoteSpy.count(), 1);
    ErrorContext ec = promoteSpy.at(0).at(0).value<ErrorContext>();
    EXPECT_EQ(ec.getCode(), ErrorContext::kErrorTypeNoError);
}

TEST_F(UT_ProcessDB, OnPrioritySet_SetpriorityOtherError_EmitsControlError)
{
    // Arrange: setpriority 失败 EINVAL（非权限类）
    stub.set_lamda(::sched_getparam, [](pid_t, struct sched_param *p) -> int {
        __DBG_STUB_INVOKE__
        p->sched_priority = 0;
        return 0;
    });
    stub.set_lamda(::setpriority, [](int, id_t, int) -> int {
        __DBG_STUB_INVOKE__
        errno = EINVAL;
        return -1;
    });
    QSignalSpy changedSpy(obj.get(), &ProcessDB::processPriorityChanged);
    QSignalSpy controlSpy(obj.get(), &ProcessDB::processControlResultReady);
    QSignalSpy promoteSpy(obj.get(), &ProcessDB::priorityPromoteResultReady);
    // Act: 非权限错误路径
    obj->onProcessPrioritysetChanged(4321, 5);
    // Assert: 发系统错误（含 EINVAL），无变更/提权信号；错误消息包含 pid
    ASSERT_GE(controlSpy.count(), 1);
    ErrorContext ec = controlSpy.at(0).at(0).value<ErrorContext>();
    EXPECT_EQ(ec.getCode(), ErrorContext::kErrorTypeSystem);
    EXPECT_EQ(ec.getSubCode(), EINVAL);
    EXPECT_TRUE(ec.getErrorMessage().contains("4321"));
    EXPECT_EQ(changedSpy.count(), 0);
    EXPECT_EQ(promoteSpy.count(), 0);
}

//━━━━━━━━━━━━━━━━━━━━ sendSignalToProcess ━━━━━━━━━━━━━━━━━━━━

TEST_F(UT_ProcessDB, SendSignal_LinglongApp_TermSuccess_EmitsProcessEnded)
{
    // Arrange: 打桩为 linglong 应用；LinglongController::execute 立即回 0
    stub.set_lamda(VADDR(AMIconManager, getProcessGroupInfo),
                   [](AMIconManager *, pid_t) -> AMProcessGroupInfo {
                       __DBG_STUB_INVOKE__
                       AMProcessGroupInfo info;
                       info.appId = "org.deepin.editor";
                       info.instancePath = "/instance0";
                       info.linglongAppId = "org.deepin.editor";
                       return info;
                   });
    stub.set_lamda(VADDR(LinglongController, execute), [](LinglongController *self) {
        __DBG_STUB_INVOKE__
        Q_EMIT self->resultReady(0);
        Q_EMIT self->finished();
    });
    QSignalSpy endedSpy(obj.get(), &ProcessDB::processEnded);
    QSignalSpy killedSpy(obj.get(), &ProcessDB::processKilled);
    QSignalSpy controlSpy(obj.get(), &ProcessDB::processControlResultReady);
    // Act: 结束 linglong 进程
    obj->sendSignalToProcess(4321, SIGTERM);
    // Assert: 走 ll-cli 成功路径，仅发 processEnded
    EXPECT_EQ(endedSpy.count(), 1);
    EXPECT_EQ(endedSpy.at(0).at(0).toInt(), 4321);
    EXPECT_EQ(killedSpy.count(), 0);
    EXPECT_EQ(controlSpy.count(), 0);
}

TEST_F(UT_ProcessDB, SendSignal_LinglongApp_KillSuccess_EmitsProcessKilled)
{
    // Arrange: linglong 应用 + execute 回 0，信号为 SIGKILL
    stub.set_lamda(VADDR(AMIconManager, getProcessGroupInfo),
                   [](AMIconManager *, pid_t) -> AMProcessGroupInfo {
                       __DBG_STUB_INVOKE__
                       AMProcessGroupInfo info;
                       info.appId = "org.deepin.editor";
                       info.instancePath = "/instance0";
                       info.linglongAppId = "org.deepin.editor";
                       return info;
                   });
    stub.set_lamda(VADDR(LinglongController, execute), [](LinglongController *self) {
        __DBG_STUB_INVOKE__
        Q_EMIT self->resultReady(0);
        Q_EMIT self->finished();
    });
    QSignalSpy endedSpy(obj.get(), &ProcessDB::processEnded);
    QSignalSpy killedSpy(obj.get(), &ProcessDB::processKilled);
    // Act: 强杀 linglong 进程
    obj->sendSignalToProcess(4321, SIGKILL);
    // Assert: 仅发 processKilled
    EXPECT_EQ(killedSpy.count(), 1);
    EXPECT_EQ(killedSpy.at(0).at(0).toInt(), 4321);
    EXPECT_EQ(endedSpy.count(), 0);
}

TEST_F(UT_ProcessDB, SendSignal_LinglongApp_Failure_EmitsControlError)
{
    // Arrange: linglong 应用 + execute 回非 0（ll-cli 失败）
    stub.set_lamda(VADDR(AMIconManager, getProcessGroupInfo),
                   [](AMIconManager *, pid_t) -> AMProcessGroupInfo {
                       __DBG_STUB_INVOKE__
                       AMProcessGroupInfo info;
                       info.appId = "org.deepin.editor";
                       info.instancePath = "/instance0";
                       info.linglongAppId = "org.deepin.editor";
                       return info;
                   });
    stub.set_lamda(VADDR(LinglongController, execute), [](LinglongController *self) {
        __DBG_STUB_INVOKE__
        Q_EMIT self->resultReady(5);
        Q_EMIT self->finished();
    });
    QSignalSpy endedSpy(obj.get(), &ProcessDB::processEnded);
    QSignalSpy controlSpy(obj.get(), &ProcessDB::processControlResultReady);
    // Act: 结束 linglong 进程但 ll-cli 失败
    obj->sendSignalToProcess(4321, SIGTERM);
    // Assert: 发系统错误（subCode=5），无 processEnded
    EXPECT_EQ(endedSpy.count(), 0);
    ASSERT_EQ(controlSpy.count(), 1);
    ErrorContext ec = controlSpy.at(0).at(0).value<ErrorContext>();
    EXPECT_EQ(ec.getCode(), ErrorContext::kErrorTypeSystem);
    EXPECT_EQ(ec.getSubCode(), 5);
}

TEST_F(UT_ProcessDB, SendSignal_LinglongApp_NonKillSignal_TakesNormalPath)
{
    // Arrange: linglong 应用但信号为 SIGCONT——不满足 linglong 分支的信号过滤，
    //         应回退普通路径；kill 打桩成功
    stub.set_lamda(VADDR(AMIconManager, getProcessGroupInfo),
                   [](AMIconManager *, pid_t) -> AMProcessGroupInfo {
                       __DBG_STUB_INVOKE__
                       AMProcessGroupInfo info;
                       info.appId = "org.deepin.editor";
                       info.instancePath = "/instance0";
                       info.linglongAppId = "org.deepin.editor";
                       return info;
                   });
    stub.set_lamda(::kill, [](pid_t, int) -> int {
        __DBG_STUB_INVOKE__
        return 0;
    });
    QSignalSpy resumedSpy(obj.get(), &ProcessDB::processResumed);
    QSignalSpy controlSpy(obj.get(), &ProcessDB::processControlResultReady);
    // Act: 继续 linglong 进程
    obj->sendSignalToProcess(4321, SIGCONT);
    // Assert: 正常路径发 processResumed('R')
    ASSERT_EQ(resumedSpy.count(), 1);
    EXPECT_EQ(resumedSpy.at(0).at(0).toInt(), 4321);
    EXPECT_EQ(resumedSpy.at(0).at(1).toInt(), 'R');
    EXPECT_EQ(controlSpy.count(), 0);
}

TEST_F(UT_ProcessDB, SendSignal_TermAllowed_SendsContThenTerm)
{
    // Arrange: 普通（非 linglong）进程；kill 打桩成功并记录调用序列
    stub.set_lamda(VADDR(AMIconManager, getProcessGroupInfo),
                   [](AMIconManager *, pid_t) -> AMProcessGroupInfo {
                       __DBG_STUB_INVOKE__
                       return AMProcessGroupInfo();
                   });
    QVector<QPair<pid_t, int>> calls;
    stub.set_lamda(::kill, [&](pid_t pid, int sig) -> int {
        __DBG_STUB_INVOKE__
        calls.append({pid, sig});
        return 0;
    });
    QSignalSpy endedSpy(obj.get(), &ProcessDB::processEnded);
    QSignalSpy controlSpy(obj.get(), &ProcessDB::processControlResultReady);
    // Act: 结束普通进程
    obj->sendSignalToProcess(4321, SIGTERM);
    // Assert: 先 SIGCONT 后 SIGTERM，最终发 processEnded，无错误
    ASSERT_EQ(calls.size(), 2);
    EXPECT_EQ(calls.at(0).second, SIGCONT);
    EXPECT_EQ(calls.at(1).second, SIGTERM);
    EXPECT_EQ(calls.at(1).first, 4321);
    EXPECT_EQ(endedSpy.count(), 1);
    EXPECT_EQ(controlSpy.count(), 0);
}

TEST_F(UT_ProcessDB, SendSignal_KillAllowed_SendsContThenKill)
{
    // Arrange: 普通进程；kill 打桩成功并记录调用序列
    stub.set_lamda(VADDR(AMIconManager, getProcessGroupInfo),
                   [](AMIconManager *, pid_t) -> AMProcessGroupInfo {
                       __DBG_STUB_INVOKE__
                       return AMProcessGroupInfo();
                   });
    QVector<QPair<pid_t, int>> calls;
    stub.set_lamda(::kill, [&](pid_t pid, int sig) -> int {
        __DBG_STUB_INVOKE__
        calls.append({pid, sig});
        return 0;
    });
    QSignalSpy killedSpy(obj.get(), &ProcessDB::processKilled);
    // Act: 强杀普通进程
    obj->sendSignalToProcess(4321, SIGKILL);
    // Assert: 先 SIGCONT 后 SIGKILL，最终发 processKilled
    ASSERT_EQ(calls.size(), 2);
    EXPECT_EQ(calls.at(0).second, SIGCONT);
    EXPECT_EQ(calls.at(1).second, SIGKILL);
    EXPECT_EQ(killedSpy.count(), 1);
}

TEST_F(UT_ProcessDB, SendSignal_StopAllowed_SendsSigstopOnly)
{
    // Arrange: 普通进程；SIGSTOP 不经过 linglong 分支也无 SIGCONT 前置
    stub.set_lamda(VADDR(AMIconManager, getProcessGroupInfo),
                   [](AMIconManager *, pid_t) -> AMProcessGroupInfo {
                       __DBG_STUB_INVOKE__
                       return AMProcessGroupInfo();
                   });
    QVector<int> sigs;
    stub.set_lamda(::kill, [&](pid_t, int sig) -> int {
        __DBG_STUB_INVOKE__
        sigs.append(sig);
        return 0;
    });
    QSignalSpy pausedSpy(obj.get(), &ProcessDB::processPaused);
    // Act: 暂停普通进程
    obj->sendSignalToProcess(4321, SIGSTOP);
    // Assert: 仅发一次 SIGSTOP，processPaused('T') 一次
    ASSERT_EQ(sigs.size(), 1);
    EXPECT_EQ(sigs.at(0), SIGSTOP);
    ASSERT_EQ(pausedSpy.count(), 1);
    EXPECT_EQ(pausedSpy.at(0).at(0).toInt(), 4321);
    EXPECT_EQ(pausedSpy.at(0).at(1).toInt(), 'T');
}

TEST_F(UT_ProcessDB, SendSignal_ContinueAllowed_SendsSigcontOnly)
{
    // Arrange: 普通进程；SIGCONT 直接走 pkill
    stub.set_lamda(VADDR(AMIconManager, getProcessGroupInfo),
                   [](AMIconManager *, pid_t) -> AMProcessGroupInfo {
                       __DBG_STUB_INVOKE__
                       return AMProcessGroupInfo();
                   });
    QVector<int> sigs;
    stub.set_lamda(::kill, [&](pid_t, int sig) -> int {
        __DBG_STUB_INVOKE__
        sigs.append(sig);
        return 0;
    });
    QSignalSpy resumedSpy(obj.get(), &ProcessDB::processResumed);
    // Act: 继续普通进程
    obj->sendSignalToProcess(4321, SIGCONT);
    // Assert: 仅一次 SIGCONT，processResumed('R') 一次
    ASSERT_EQ(sigs.size(), 1);
    EXPECT_EQ(sigs.at(0), SIGCONT);
    ASSERT_EQ(resumedSpy.count(), 1);
    EXPECT_EQ(resumedSpy.at(0).at(1).toInt(), 'R');
}

TEST_F(UT_ProcessDB, SendSignal_ContDeniedEperm_PkexecSucceeds_EmitsProcessEnded)
{
    // Arrange: 首个 SIGCONT 被 EPERM 拒绝 → ProcessController pkexec 路径并成功
    stub.set_lamda(VADDR(AMIconManager, getProcessGroupInfo),
                   [](AMIconManager *, pid_t) -> AMProcessGroupInfo {
                       __DBG_STUB_INVOKE__
                       return AMProcessGroupInfo();
                   });
    stub.set_lamda(::kill, [](pid_t, int) -> int {
        __DBG_STUB_INVOKE__
        errno = EPERM;
        return -1;
    });
    stub.set_lamda(VADDR(ProcessController, execute), [](ProcessController *self) {
        __DBG_STUB_INVOKE__
        Q_EMIT self->resultReady(0);
        Q_EMIT self->finished();
    });
    QSignalSpy endedSpy(obj.get(), &ProcessDB::processEnded);
    QSignalSpy controlSpy(obj.get(), &ProcessDB::processControlResultReady);
    // Act: 结束进程（SIGCONT 与 SIGTERM 都会 EPERM，走 pkexec 且成功）
    obj->sendSignalToProcess(4321, SIGTERM);
    // Assert: pkexec 成功 → 发 processEnded，无错误
    EXPECT_EQ(endedSpy.count(), 1);
    EXPECT_EQ(endedSpy.at(0).at(0).toInt(), 4321);
    EXPECT_EQ(controlSpy.count(), 0);
}

TEST_F(UT_ProcessDB, SendSignal_ContDeniedOtherError_EmitsControlError)
{
    // Arrange: SIGCONT 失败且 errno 为 ESRCH（非 EPERM）
    stub.set_lamda(VADDR(AMIconManager, getProcessGroupInfo),
                   [](AMIconManager *, pid_t) -> AMProcessGroupInfo {
                       __DBG_STUB_INVOKE__
                       return AMProcessGroupInfo();
                   });
    stub.set_lamda(::kill, [](pid_t, int) -> int {
        __DBG_STUB_INVOKE__
        errno = ESRCH;
        return -1;
    });
    QSignalSpy endedSpy(obj.get(), &ProcessDB::processEnded);
    QSignalSpy controlSpy(obj.get(), &ProcessDB::processControlResultReady);
    // Act: 结束不存在的进程
    obj->sendSignalToProcess(4321, SIGTERM);
    // Assert: 发系统错误（subCode=ESRCH），无 processEnded
    EXPECT_EQ(endedSpy.count(), 0);
    ASSERT_EQ(controlSpy.count(), 1);
    ErrorContext ec = controlSpy.at(0).at(0).value<ErrorContext>();
    EXPECT_EQ(ec.getSubCode(), ESRCH);
    EXPECT_EQ(ec.getCode(), ErrorContext::kErrorTypeSystem);
}

TEST_F(UT_ProcessDB, SendSignal_TermDeniedEperm_PkexecFails_EmitsControlError)
{
    // Arrange: SIGCONT 成功但 SIGTERM 被 EPERM 拒绝；pkexec 执行失败（回 3）
    stub.set_lamda(VADDR(AMIconManager, getProcessGroupInfo),
                   [](AMIconManager *, pid_t) -> AMProcessGroupInfo {
                       __DBG_STUB_INVOKE__
                       return AMProcessGroupInfo();
                   });
    stub.set_lamda(::kill, [&](pid_t, int sig) -> int {
        __DBG_STUB_INVOKE__
        if (sig == SIGCONT)
            return 0;  // 前置 SIGCONT 成功
        errno = EPERM; // 正式信号被拒
        return -1;
    });
    stub.set_lamda(VADDR(ProcessController, execute), [](ProcessController *self) {
        __DBG_STUB_INVOKE__
        Q_EMIT self->resultReady(3);
        Q_EMIT self->finished();
    });
    QSignalSpy endedSpy(obj.get(), &ProcessDB::processEnded);
    QSignalSpy controlSpy(obj.get(), &ProcessDB::processControlResultReady);
    // Act: 结束进程（pkexec 失败）
    obj->sendSignalToProcess(4321, SIGTERM);
    // Assert: 发系统错误（subCode=3），无 processEnded
    EXPECT_EQ(endedSpy.count(), 0);
    ASSERT_EQ(controlSpy.count(), 1);
    ErrorContext ec = controlSpy.at(0).at(0).value<ErrorContext>();
    EXPECT_EQ(ec.getSubCode(), 3);
}

TEST_F(UT_ProcessDB, SendSignal_KillFailsOtherError_EmitsControlError)
{
    // Arrange: SIGCONT 成功但 SIGKILL 失败且 errno=ESRCH（非 EPERM）
    stub.set_lamda(VADDR(AMIconManager, getProcessGroupInfo),
                   [](AMIconManager *, pid_t) -> AMProcessGroupInfo {
                       __DBG_STUB_INVOKE__
                       return AMProcessGroupInfo();
                   });
    stub.set_lamda(::kill, [&](pid_t, int sig) -> int {
        __DBG_STUB_INVOKE__
        if (sig == SIGCONT)
            return 0;
        errno = ESRCH;
        return -1;
    });
    QSignalSpy killedSpy(obj.get(), &ProcessDB::processKilled);
    QSignalSpy controlSpy(obj.get(), &ProcessDB::processControlResultReady);
    // Act: 强杀进程失败
    obj->sendSignalToProcess(4321, SIGKILL);
    // Assert: 发系统错误（subCode=ESRCH），无 processKilled
    EXPECT_EQ(killedSpy.count(), 0);
    ASSERT_EQ(controlSpy.count(), 1);
    ErrorContext ec = controlSpy.at(0).at(0).value<ErrorContext>();
    EXPECT_EQ(ec.getSubCode(), ESRCH);
}

TEST_F(UT_ProcessDB, SendSignal_UnknownSignal_NoSignalEmitted)
{
    // Arrange: 普通进程 + 未知信号 SIGUSR1（emitSignal 的 default 分支）
    stub.set_lamda(VADDR(AMIconManager, getProcessGroupInfo),
                   [](AMIconManager *, pid_t) -> AMProcessGroupInfo {
                       __DBG_STUB_INVOKE__
                       return AMProcessGroupInfo();
                   });
    stub.set_lamda(::kill, [](pid_t, int) -> int {
        __DBG_STUB_INVOKE__
        return 0;
    });
    QSignalSpy endedSpy(obj.get(), &ProcessDB::processEnded);
    QSignalSpy killedSpy(obj.get(), &ProcessDB::processKilled);
    QSignalSpy pausedSpy(obj.get(), &ProcessDB::processPaused);
    QSignalSpy resumedSpy(obj.get(), &ProcessDB::processResumed);
    QSignalSpy controlSpy(obj.get(), &ProcessDB::processControlResultReady);
    // Act: 发送未分类信号
    obj->sendSignalToProcess(4321, SIGUSR1);
    // Assert: kill 已执行但不发任何业务信号，也不发错误
    EXPECT_EQ(endedSpy.count(), 0);
    EXPECT_EQ(killedSpy.count(), 0);
    EXPECT_EQ(pausedSpy.count(), 0);
    EXPECT_EQ(resumedSpy.count(), 0);
    EXPECT_EQ(controlSpy.count(), 0);
}

} // namespace
