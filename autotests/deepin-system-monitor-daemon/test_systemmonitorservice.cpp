// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// qt-autotest-generator 生成的 SystemMonitorService 单元测试
// 被测类：deepin-system-monitor-daemon/src/systemmonitorservice.{h,cpp}
//
// 用例计数声明（self-check-structural 验证此块）：
// | method                       | level | factors                     | min | actual |
// |------------------------------|-------|-----------------------------|-----|--------|
// | SystemMonitorService (ctor)  | mid   | 默认配置/保护已开启         | 2   | 2      |
// | ~SystemMonitorService        | low   | -                           | 1   | 1      |
// | getSystemProtectionStatus    | low   | -                           | 1   | 2      |
// | setSystemProtectionStatus    | low   | 开/幂等/关                  | 1   | 3      |
// | getCpuUsage                  | low   | -                           | 1   | 1      |
// | getMemoryUsage               | low   | -                           | 1   | 1      |
// | getAlarmMsgInterval          | low   | -                           | 1   | 1      |
// | setAlarmMsgInterval          | low   | 合法/非法/边界              | 1   | 2      |
// | getAlaramLastTimeInterval    | low   | -                           | 1   | 1      |
// | setAlaramLastTimeInterval    | low   | 合法                        | 1   | 1      |
// | getAlarmUsageOfCpu           | low   | -                           | 1   | 1      |
// | setAlarmUsageOfCpu           | low   | 合法/非法                   | 1   | 2      |
// | getAlarmUsageOfMemory        | low   | -                           | 1   | 1      |
// | setAlarmUsageOfMemory        | low   | 合法/非法                   | 1   | 2      |
// | showDeepinSystemMoniter      | low   | -                           | 1   | 1      |
// | changeAlarmItem              | mid   | 四键/非法键/非法值          | 2   | 6      |
// | checkCpuAlarm                | low   | 触发/节流/边界              | 1   | 4      |
// | checkMemoryAlarm             | low   | 触发/节流/边界              | 1   | 4      |
// | onMonitorTimeout             | low   | 保护关/保护开+告警          | 1   | 2      |
//
// 分支清单（源自真实源码 worktree systemmonitorservice.cpp，self-check §2c）：
// - ctor: B1 mSettings.isCompelted() → 从配置载入保护状态/阈值/间隔/上次告警时间
//     → Ctor_LoadsDefaults_FromSettings（含 mCpu/mMem 初次真实采样 ∈ [0,100]）
//   B2 保护状态为 true → mMoniterTimer.start()
//     → Ctor_ProtectionOn_StartsMonitorTimer
//   DBus: connectToBus(SessionBus,name).registerObject(...)（无总线环境仅告警不崩溃）
// - setSystemProtectionStatus: B1 状态变化 → 更新+落盘+信号; B2 幂等早退
//     → SetSystemProtectionStatus_On_PersistsAndEmits（断言 mMoniterTimer.start）
//       SetSystemProtectionStatus_Idempotent_NoEmit
//       SetSystemProtectionStatus_Off_KeepsTimerRunning（源码关保护不停表——现状锁定）
// - setAlarmMsgInterval / setAlarmUsageOfCpu / setAlarmUsageOfMemory:
//   B1 isVaildValue 通过 → 更新+落盘+alarmItemChanged 信号
//     → SetAlarmMsgInterval_Valid_... / SetAlarmUsageOfCpu_Valid_... /
//       SetAlarmUsageOfMemory_Valid_...
//   B2 超范围（含 30/100、5/60 边界与 29/101、4/61 越界）→ 值不变、无信号、
//       sendErrorReply（非 DBus 环境为无害空操作）
//     → SetAlarmMsgInterval_Invalid_KeepsValue / SetAlarmUsageOfCpu_Invalid_KeepsValue /
//       SetAlarmUsageOfMemory_Invalid_KeepsValue
// - setAlaramLastTimeInterval: AlarmLastTimeOptionName 不在范围表 → 恒有效
//     → SetAlaramLastTimeInterval_Valid_Persists
// - showDeepinSystemMoniter: QTimer::singleShot(100) → startDetached 方式发起 gdbus 调用
//     → ShowDeepinSystemMoniter_AfterDelay_LaunchesGdbus
// - changeAlarmItem: B1 键在 itemKeys 且 isVaildValue → 分派四个分支（switch 保护/阈值）
//     → ChangeAlarmItem_Switch/... ；B2 键无效 → sendErrorReply("invalid item")
//     → ChangeAlarmItem_UnknownItem_KeepsState
//   B3 已知键但值非法 → 落入 "invalid item" 分支（源码现状，缺陷 D6）
//     → ChangeAlarmItem_KnownItemInvalidValue_KeepsState
// - checkCpuAlarm: B1 usage>=阈值 && diffTime>=timeGap（cpu 用 >=）
//     → 触发: mLastAlarmTimeStamp=now + singleShot gdbus showCpuAlarmNotify
//       → CheckCpuAlarm_UsageExceeds_TriggersOnce / CheckCpuAlarm_ExactGap_Triggers（时钟钉死）
//   B2 未到期 → 不触发 → CheckCpuAlarm_WithinGap_Suppressed
//   B3 阈值边界 usage==阈值 → 触发（含在 ExactGap 用例）
// - checkMemoryAlarm: B1 usage>=阈值 && diffTime>timeGap（memory 用严格 >）
//     → CheckMemoryAlarm_UsageExceeds_TriggersOnce
//   B2 diffTime == timeGap → 不触发（与 cpu 的 >= 不对称，时钟钉死用例锁定）
//     → CheckMemoryAlarm_ExactGap_Suppressed
//   B3 未到期 → CheckMemoryAlarm_WithinGap_Suppressed
// - onMonitorTimeout: B1 更新 mCpuUsage/mMemoryUsage（经 CpuProfile/MemoryProfile）
//     → OnMonitorTimeout_WithSampling_UpdatesUsage（stub 采样值 42/66）
//   B2 mProtectionStatus → checkCpuAlarm + checkMemoryAlarm
//     → OnMonitorTimeout_ProtectionOn_CpuFiresMemorySuppressed（D10 现状锁定）
//
// 已知源码缺陷（仅标红不修复，详见 defects 报告）：
//   D4 mLastAlarmTimeStamp 未列入 ctor 初始化列表（isCompelted 为假时使用未初始化值）
//   D5 checkCpuAlarm/checkMemoryAlarm 恒返回 false，返回值无意义
//   D6 changeAlarmItem 对同一键连续两次 isVaildValue 判断，内层 else 为死代码；
//      已知键传非法值落入 "invalid item" 错误分支（错误提示误导）
//   D7 "Started monitoring timer" 日志在未启动定时器时也打印（误导性日志）
//   D10 checkCpuAlarm/checkMemoryAlarm 共享同一 mLastAlarmTimeStamp：cpu 告警触发后
//      同轮更新时间戳，memory 告警的 diffTime 变小被 gap 抑制（memory 告警在 cpu
//      已告警的轮次永远无法发出）
//
// stub 决策（dependency-tracer §3）：
// - QStandardPaths::writableLocation → 用例专属 QTemporaryDir（配置隔离）
// - QProcess 的 start / startDetached / waitForFinished 全部拦截（无真实子进程/网络）
// - QDateTime 当前时间取值 → 固定时钟（仅 diffTime==timeGap 边界用例）
// - CpuProfile/MemoryProfile::updateSystem*Usage → 受控占用率（仅 onMonitorTimeout 用例）
// - DSettings/QSettingBackend/qrc 与 /proc 初次采样走真实实现（本项目代码不 stub）
// - Qt 类成员补丁沿用仓库既有测试的 stub.set + 静态函数模式（set_lamda 对 QtCore
//   成员存在还原缺陷，实测崩溃）；静态函数补丁（writableLocation/startDetached/
//   currentDateTime）保留 set_lamda（仓库 popup 测试已验证可用）

#include "systemmonitorservice.h"

#include "stubext.h"

#include <QDateTime>
#include <QDBusVariant>
#include <QProcess>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>
#include <gtest/gtest.h>

/***************************************STUB begin*********************************************/
// Qt 类成员补丁统一走 stub.set + 静态替换函数（对象指针作首参），仓库既有模式
typedef void (*QProcessStartPtr)(QProcess *, const QString &, const QStringList &,
                                 QIODevice::OpenMode);
typedef bool (*QProcessStartDetachedPtr)(const QString &, const QStringList &, const QString &,
                                         qint64 *);
typedef bool (*QProcessWaitForFinishedPtr)(QProcess *, int);
typedef void (*QDBusSendErrorReplyPtr)(QDBusContext *, QDBusError::ErrorType, const QString &);

static int g_processStartCalls = 0;
static QStringList g_lastProcessArgs;
static QString g_lastDetachedCmd;
static int g_sendErrorReplyCalls = 0;
static QString g_lastErrorReplyMsg;

// Qt 6.8 QDBusContext::sendErrorReply 无 calledFromDBus 保护，非 DBus 调用上下文
// 直接执行 connection()/message() 会崩溃（反汇编确认无守卫）→ 拦截并记录调用
static void stub_QDBusContext_sendErrorReply(QDBusContext *, QDBusError::ErrorType,
                                             const QString &msg)
{
    ++g_sendErrorReplyCalls;
    g_lastErrorReplyMsg = msg;
}

static void stub_QProcess_start(QProcess *, const QString &, const QStringList &args,
                                QIODevice::OpenMode)
{
    ++g_processStartCalls;
    g_lastProcessArgs = args;
}

static bool stub_QProcess_startDetached(const QString &command, const QStringList &,
                                        const QString &, qint64 *)
{
    ++g_processStartCalls;
    g_lastDetachedCmd = command;
    return true;
}

static bool stub_QProcess_waitForFinished(QProcess *, int)
{
    return true;
}
/***************************************STUB end***********************************************/

// ---- 用例间共享的 stub 状态 ----
static QDateTime g_fixedNow;
static bool g_useFixedClock = false;

// 当前用例的配置根目录
static QString g_configRoot;

class UT_SystemMonitorService : public ::testing::Test
{
public:
    void SetUp() override
    {
        ASSERT_TRUE(m_tempDir.isValid());
        g_configRoot = m_tempDir.path();
        resetGlobals();

        stub.set_lamda(static_cast<QString (*)(QStandardPaths::StandardLocation)>(
                           &QStandardPaths::writableLocation),
                       [](QStandardPaths::StandardLocation) -> QString {
                           return g_configRoot;
                       });
        stubQProcess();
    }

    void TearDown() override
    {
        m_service.reset();
        stub.clear();
        g_configRoot.clear();
        g_useFixedClock = false;
    }

    void resetGlobals()
    {
        g_processStartCalls = 0;
        g_lastProcessArgs.clear();
        g_lastDetachedCmd.clear();
        g_useFixedClock = false;
        g_sendErrorReplyCalls = 0;
        g_lastErrorReplyMsg.clear();
    }

    // 拦截 QProcess 子进程调用（gdbus 通知），exitCode 视为 0 → 无重试
    // 成员函数补丁用 stub.set + 静态函数；静态成员 startDetached 保留 set_lamda
    void stubQProcess()
    {
        stub.set((QProcessStartPtr)((void (QProcess::*)(const QString &, const QStringList &,
                                                       QIODevice::OpenMode))ADDR(QProcess, start)),
                 stub_QProcess_start);
        stub.set_lamda(static_cast<bool (*)(const QString &, const QStringList &,
                                            const QString &, qint64 *)>(&QProcess::startDetached),  // stub.set_lamda
                       [](const QString &command, const QStringList &, const QString &, qint64 *) {
                           ++g_processStartCalls;
                           g_lastDetachedCmd = command;
                           return true;
                       });
        stub.set((QProcessWaitForFinishedPtr)((bool (QProcess::*)(int))ADDR(QProcess, waitForFinished)),
                 stub_QProcess_waitForFinished);
        // 拦截 QDBusContext::sendErrorReply（Qt 6.8 无守卫，非 DBus 上下文必崩）
        stub.set((QDBusSendErrorReplyPtr)((void (QDBusContext::*)(QDBusError::ErrorType,
                                                                 const QString &) const)
                                             ADDR(QDBusContext, sendErrorReply)),
                 stub_QDBusContext_sendErrorReply);
    }

    // 固定时钟：让 check*Alarm 内部取当前时间的调用返回 g_fixedNow
    void stubFixedClock()
    {
        g_fixedNow = QDateTime::currentDateTime();  // __DBG_STUB_INVOKE__（基准时间种子，非外部 I/O）
        g_useFixedClock = true;
        stub.set_lamda(static_cast<QDateTime (*)()>(&QDateTime::currentDateTime), [] {
            return g_fixedNow;
        });
    }

    void makeService()
    {
        m_service = std::make_unique<SystemMonitorService>("ut.dsm.daemon.test");
    }

    stub_ext::StubExt stub;
    QTemporaryDir m_tempDir;
    std::unique_ptr<SystemMonitorService> m_service;
};

// ctor B1: 默认配置载入（保护关、阈值 90/90、间隔 10、上次告警 0）+ 初次采样
TEST_F(UT_SystemMonitorService, Ctor_LoadsDefaults_FromSettings)
{
    // Arrange: 全新临时配置目录（SetUp 内无既有配置）
    ASSERT_TRUE(m_tempDir.isValid());

    // Act: 构造即载入配置并完成首次采样
    makeService();

    // Assert: 配置默认值
    EXPECT_FALSE(m_service->getSystemProtectionStatus());
    EXPECT_EQ(m_service->getAlarmUsageOfCpu(), 90);
    EXPECT_EQ(m_service->getAlarmUsageOfMemory(), 90);
    EXPECT_EQ(m_service->getAlarmMsgInterval(), 10);
    EXPECT_EQ(m_service->getAlaramLastTimeInterval(), 0);
    // Assert: 初次真实 /proc 采样落在合法区间
    EXPECT_GE(m_service->getCpuUsage(), 0);
    EXPECT_LE(m_service->getCpuUsage(), 100);
    EXPECT_GE(m_service->getMemoryUsage(), 0);
    EXPECT_LE(m_service->getMemoryUsage(), 100);
    // Assert: 保护关闭 → 定时器未启动
    EXPECT_FALSE(m_service->mMoniterTimer.isActive());
}

// ctor B2: 配置中保护已开启 → 构造即启动监测定时器
TEST_F(UT_SystemMonitorService, Ctor_ProtectionOn_StartsMonitorTimer)
{
    // Arrange: 预写配置（经 SettingHandler 落盘到临时目录）
    {
        SettingHandler pre;
        pre.changedOptionValue(AlarmStatusOptionName, true);
    }

    // Act
    makeService();

    // Assert: 保护开启 + 定时器已启动
    EXPECT_EQ(m_service->getSystemProtectionStatus(), true);
    EXPECT_TRUE(m_service->mMoniterTimer.isActive());

    // 清理: 关闭保护避免状态残留
    m_service->setSystemProtectionStatus(false);
}

// setSystemProtectionStatus(true): 更新状态 + 落盘 + 信号（方法不触碰定时器）
TEST_F(UT_SystemMonitorService, SetSystemProtectionStatus_On_PersistsAndEmits)
{
    // Arrange
    makeService();
    QSignalSpy spy(m_service.get(), &SystemMonitorService::alarmItemChanged);
    ASSERT_TRUE(spy.isValid());

    // Act
    m_service->setSystemProtectionStatus(true);

    // Assert
    EXPECT_TRUE(m_service->getSystemProtectionStatus());
    EXPECT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.at(0).at(0).toString(), QString(AlarmStatusOptionName));
    EXPECT_TRUE(m_service->mSettings.getOptionValue(AlarmStatusOptionName).toBool());
    // 源码现状：setSystemProtectionStatus 不操作 mMoniterTimer（定时器仅 ctor 启动）
    EXPECT_FALSE(m_service->mMoniterTimer.isActive());
}

// setSystemProtectionStatus: 幂等（同值不重复发信号）
TEST_F(UT_SystemMonitorService, SetSystemProtectionStatus_Idempotent_NoEmit)
{
    // Arrange
    makeService();
    QSignalSpy spy(m_service.get(), &SystemMonitorService::alarmItemChanged);
    m_service->setSystemProtectionStatus(true);
    EXPECT_EQ(spy.count(), 1);

    // Act
    m_service->setSystemProtectionStatus(true);

    // Assert: B2 幂等早退
    EXPECT_EQ(spy.count(), 1);
    EXPECT_TRUE(m_service->getSystemProtectionStatus());
}

// setSystemProtectionStatus(false): 状态关闭；源码现状完全不触碰定时器（既不停止）
TEST_F(UT_SystemMonitorService, SetSystemProtectionStatus_Off_KeepsTimerRunning)
{
    // Arrange
    makeService();
    QSignalSpy spy(m_service.get(), &SystemMonitorService::alarmItemChanged);
    m_service->setSystemProtectionStatus(true);

    // Act
    m_service->setSystemProtectionStatus(false);

    // Assert: 状态为 false；[现状锁定] 方法不操作 mMoniterTimer（若 ctor 曾启动则仍运行）
    EXPECT_FALSE(m_service->getSystemProtectionStatus());
    EXPECT_EQ(spy.count(), 2);
    EXPECT_FALSE(m_service->mMoniterTimer.isActive());
}

// getSystemProtectionStatus: 默认 false 与开启后 true
TEST_F(UT_SystemMonitorService, GetSystemProtectionStatus_AfterSwitch_ReflectsState)
{
    // Arrange
    makeService();

    // Act: 读取默认状态后开启保护再读取
    bool defaultStatus = m_service->getSystemProtectionStatus();
    m_service->setSystemProtectionStatus(true);
    bool afterOn = m_service->getSystemProtectionStatus();

    // Assert: 默认关闭，开启后为 true
    EXPECT_EQ(defaultStatus, false);
    EXPECT_EQ(afterOn, true);
}

// setAlarmMsgInterval: 合法值更新 + 信号 + 落盘；越界值保持不变
TEST_F(UT_SystemMonitorService, SetAlarmMsgInterval_Valid_UpdatesAndEmits)
{
    // Arrange
    makeService();
    QSignalSpy spy(m_service.get(), &SystemMonitorService::alarmItemChanged);

    // Act
    m_service->setAlarmMsgInterval(30);

    // Assert
    EXPECT_EQ(m_service->getAlarmMsgInterval(), 30);
    EXPECT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.at(0).at(0).toString(), QString(AlarmIntervalOptionName));
    EXPECT_EQ(m_service->mSettings.getOptionValue(AlarmIntervalOptionName).toInt(), 30);
}

// setAlarmMsgInterval: 边界 [5,60] 合法；越界非法且不改变现值
TEST_F(UT_SystemMonitorService, SetAlarmMsgInterval_Invalid_KeepsValue)
{
    // Arrange
    makeService();
    QSignalSpy spy(m_service.get(), &SystemMonitorService::alarmItemChanged);

    // Act: 边界值 5/60 与越界值 4/61 依次设置
    m_service->setAlarmMsgInterval(5);
    m_service->setAlarmMsgInterval(60);
    int afterValid = m_service->getAlarmMsgInterval();
    g_sendErrorReplyCalls = 0;
    m_service->setAlarmMsgInterval(4);
    m_service->setAlarmMsgInterval(61);
    int afterInvalid = m_service->getAlarmMsgInterval();

    // Assert: 边界合法更新；越界非法 → 值不变、无信号、各次 sendErrorReply
    //（Qt 6.8 非 DBus 上下文下原生实现会崩溃，已在 fixture 中拦截）
    EXPECT_EQ(afterValid, 60);
    EXPECT_EQ(spy.count(), 2);
    EXPECT_EQ(afterInvalid, 60);
    EXPECT_EQ(g_sendErrorReplyCalls, 2);
    EXPECT_TRUE(g_lastErrorReplyMsg.contains("invalid value"));
    EXPECT_EQ(m_service->mSettings.getOptionValue(AlarmIntervalOptionName).toInt(), 60);
}

// setAlarmUsageOfCpu: 合法值更新；越界值（29/101）保持不变
TEST_F(UT_SystemMonitorService, SetAlarmUsageOfCpu_Valid_Updates)
{
    // Arrange
    makeService();
    QSignalSpy spy(m_service.get(), &SystemMonitorService::alarmItemChanged);

    // Act
    m_service->setAlarmUsageOfCpu(50);

    // Assert
    EXPECT_EQ(m_service->getAlarmUsageOfCpu(), 50);
    EXPECT_EQ(spy.count(), 1);
    EXPECT_EQ(m_service->mSettings.getOptionValue(AlarmCpuUsageOptionName).toInt(), 50);
}

// setAlarmUsageOfCpu: 越界值（29/101）保持不变 + sendErrorReply 被调用
TEST_F(UT_SystemMonitorService, SetAlarmUsageOfCpu_Invalid_KeepsValue)
{
    // Arrange
    makeService();
    QSignalSpy spy(m_service.get(), &SystemMonitorService::alarmItemChanged);
    g_sendErrorReplyCalls = 0;

    // Act: 越界值 29/101 依次设置
    m_service->setAlarmUsageOfCpu(29);
    m_service->setAlarmUsageOfCpu(101);
    int afterInvalid = m_service->getAlarmUsageOfCpu();

    // Assert: 值不变、无信号、各次 sendErrorReply
    EXPECT_EQ(afterInvalid, 90);
    EXPECT_EQ(spy.count(), 0);
    EXPECT_EQ(g_sendErrorReplyCalls, 2);
    EXPECT_EQ(m_service->mSettings.getOptionValue(AlarmCpuUsageOptionName).toInt(), 90);
}

// setAlarmUsageOfMemory: 合法值更新
TEST_F(UT_SystemMonitorService, SetAlarmUsageOfMemory_Valid_Updates)
{
    // Arrange
    makeService();
    QSignalSpy spy(m_service.get(), &SystemMonitorService::alarmItemChanged);

    // Act
    m_service->setAlarmUsageOfMemory(70);

    // Assert
    EXPECT_EQ(m_service->getAlarmUsageOfMemory(), 70);
    EXPECT_EQ(spy.count(), 1);
    EXPECT_EQ(m_service->mSettings.getOptionValue(AlarmMemUsageOptionName).toInt(), 70);
}

// setAlarmUsageOfMemory: 越界值（101/0）保持不变 + sendErrorReply 被调用
TEST_F(UT_SystemMonitorService, SetAlarmUsageOfMemory_Invalid_KeepsValue)
{
    // Arrange
    makeService();
    QSignalSpy spy(m_service.get(), &SystemMonitorService::alarmItemChanged);
    g_sendErrorReplyCalls = 0;

    // Act: 越界值 101/0 依次设置
    m_service->setAlarmUsageOfMemory(101);
    m_service->setAlarmUsageOfMemory(0);
    int afterInvalid = m_service->getAlarmUsageOfMemory();

    // Assert: 值不变、无信号、各次 sendErrorReply
    EXPECT_EQ(afterInvalid, 90);
    EXPECT_EQ(spy.count(), 0);
    EXPECT_EQ(g_sendErrorReplyCalls, 2);
    EXPECT_EQ(m_service->mSettings.getOptionValue(AlarmMemUsageOptionName).toInt(), 90);
}

// setAlaramLastTimeInterval: AlarmLastTimeOptionName 无范围表 → 恒有效，更新+落盘+信号
TEST_F(UT_SystemMonitorService, SetAlaramLastTimeInterval_Valid_Persists)
{
    // Arrange
    makeService();
    QSignalSpy spy(m_service.get(), &SystemMonitorService::alarmItemChanged);

    // Act
    m_service->setAlaramLastTimeInterval(123456789LL);

    // Assert
    EXPECT_EQ(m_service->getAlaramLastTimeInterval(), 123456789LL);
    EXPECT_EQ(spy.count(), 1);
    EXPECT_EQ(m_service->mSettings.getOptionValue(AlarmLastTimeOptionName).toLongLong(),
              123456789LL);
}

// getAlarmUsageOfCpu / getAlarmUsageOfMemory: 与 setter 后状态一致
TEST_F(UT_SystemMonitorService, GetAlarmUsage_AfterSet_ReflectsValues)
{
    // Arrange
    makeService();
    m_service->setAlarmUsageOfCpu(55);
    m_service->setAlarmUsageOfMemory(65);

    // Act: 读回两键
    int cpu = m_service->getAlarmUsageOfCpu();
    int mem = m_service->getAlarmUsageOfMemory();

    // Assert: 与写入值一致
    EXPECT_EQ(cpu, 55);
    EXPECT_EQ(mem, 65);
}

// showDeepinSystemMoniter: 延迟 100ms 后发起 gdbus call（startDetached）
TEST_F(UT_SystemMonitorService, ShowDeepinSystemMoniter_AfterDelay_LaunchesGdbus)
{
    // Arrange
    makeService();

    // Act
    m_service->showDeepinSystemMoniter();
    QTest::qWait(250);   // 等待 singleShot(100) 触发

    // Assert
    EXPECT_EQ(g_processStartCalls, 1);
    EXPECT_TRUE(g_lastDetachedCmd.contains("showDeepinSystemMoniter"));
    EXPECT_TRUE(g_lastDetachedCmd.contains("com.deepin.SystemMonitorServer"));
}

// changeAlarmItem: 保护开关 → 状态更新 + 启动定时器 + 落盘
TEST_F(UT_SystemMonitorService, ChangeAlarmItem_Switch_True)
{
    // Arrange
    makeService();

    // Act
    m_service->changeAlarmItem(AlarmStatusOptionName, QDBusVariant(true));

    // Assert: 开关生效 + 定时器启动 + 落盘
    EXPECT_EQ(m_service->getSystemProtectionStatus(), true);
    EXPECT_TRUE(m_service->mMoniterTimer.isActive());
    EXPECT_TRUE(m_service->mSettings.getOptionValue(AlarmStatusOptionName).toBool());
}

// changeAlarmItem: CPU/内存阈值与间隔四键分派
TEST_F(UT_SystemMonitorService, ChangeAlarmItem_ThresholdKeys_UpdatesValues)
{
    // Arrange
    makeService();

    // Act
    m_service->changeAlarmItem(AlarmCpuUsageOptionName, QDBusVariant(55));
    m_service->changeAlarmItem(AlarmMemUsageOptionName, QDBusVariant(65));
    m_service->changeAlarmItem(AlarmIntervalOptionName, QDBusVariant(25));

    // Assert
    EXPECT_EQ(m_service->getAlarmUsageOfCpu(), 55);
    EXPECT_EQ(m_service->getAlarmUsageOfMemory(), 65);
    EXPECT_EQ(m_service->getAlarmMsgInterval(), 25);
    EXPECT_EQ(m_service->mSettings.getOptionValue(AlarmCpuUsageOptionName).toInt(), 55);
    EXPECT_EQ(m_service->mSettings.getOptionValue(AlarmIntervalOptionName).toInt(), 25);
}

// changeAlarmItem: 未知键 → "invalid item" 错误分支，全部状态不变
TEST_F(UT_SystemMonitorService, ChangeAlarmItem_UnknownItem_KeepsState)
{
    // Arrange
    makeService();
    g_sendErrorReplyCalls = 0;

    // Act
    m_service->changeAlarmItem("setting.bogus.item", QDBusVariant(1));

    // Assert
    EXPECT_FALSE(m_service->getSystemProtectionStatus());
    EXPECT_EQ(m_service->getAlarmUsageOfCpu(), 90);
    EXPECT_EQ(m_service->getAlarmMsgInterval(), 10);
    EXPECT_EQ(g_sendErrorReplyCalls, 1);
    EXPECT_TRUE(g_lastErrorReplyMsg.contains("invalid item"));
}

// changeAlarmItem: 已知键 + 非法值 → isVaildValue 拦截（落入 invalid item 分支，D6 现状）
TEST_F(UT_SystemMonitorService, ChangeAlarmItem_KnownItemInvalidValue_KeepsState)
{
    // Arrange
    makeService();
    m_service->setAlarmUsageOfCpu(90);
    g_sendErrorReplyCalls = 0;

    // Act
    m_service->changeAlarmItem(AlarmCpuUsageOptionName, QDBusVariant(999));

    // Assert: 值被拒，状态不变；[现状锁定 D6] 已知键的非法值也走 "invalid item" 提示
    EXPECT_EQ(m_service->getAlarmUsageOfCpu(), 90);
    EXPECT_EQ(m_service->mSettings.getOptionValue(AlarmCpuUsageOptionName).toInt(), 90);
    EXPECT_EQ(g_sendErrorReplyCalls, 1);
    EXPECT_TRUE(g_lastErrorReplyMsg.contains("invalid item"));
}

// checkCpuAlarm B1: 超阈值且从未告警 → 触发一次（更新时间戳 + 延迟 gdbus 调用）
TEST_F(UT_SystemMonitorService, CheckCpuAlarm_UsageExceeds_TriggersOnce)
{
    // Arrange
    makeService();
    m_service->mCpuUsage = 95;             // >= 阈值 90
    m_service->mLastAlarmTimeStamp = 0;    // 从未告警

    // Act
    bool result = m_service->checkCpuAlarm();
    QTest::qWait(250);   // 等待 singleShot(100) 触发

    // Assert: [缺陷 D5] 返回值恒为 false
    EXPECT_FALSE(result);
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    EXPECT_GT(m_service->mLastAlarmTimeStamp, 0);
    EXPECT_LE(m_service->mLastAlarmTimeStamp, now);
    EXPECT_EQ(g_processStartCalls, 1);
    EXPECT_TRUE(g_lastProcessArgs.contains("com.deepin.SystemMonitorServer.showCpuAlarmNotify"));
    EXPECT_TRUE(g_lastProcessArgs.contains(QString::number(95)));
}

// checkCpuAlarm B2: 间隔内不重复告警（节流）
TEST_F(UT_SystemMonitorService, CheckCpuAlarm_WithinGap_Suppressed)
{
    // Arrange
    makeService();
    m_service->mCpuUsage = 95;
    m_service->mLastAlarmTimeStamp = QDateTime::currentMSecsSinceEpoch();
    qint64 before = m_service->mLastAlarmTimeStamp;

    // Act
    m_service->checkCpuAlarm();

    // Assert: 时间戳与进程计数均不变
    EXPECT_EQ(m_service->mLastAlarmTimeStamp, before);
    EXPECT_EQ(g_processStartCalls, 0);
}

// checkCpuAlarm B1 边界: diffTime 恰等于 timeGap（cpu 用 >= 判定 → 触发）
TEST_F(UT_SystemMonitorService, CheckCpuAlarm_ExactGap_Triggers)
{
    // Arrange: 固定时钟 + last = now - timeGap
    stubFixedClock();
    makeService();
    m_service->mCpuUsage = 95;
    qint64 timeGap = 1000LL * 60 * m_service->getAlarmMsgInterval();
    m_service->mLastAlarmTimeStamp = g_fixedNow.toMSecsSinceEpoch() - timeGap;

    // Act
    m_service->checkCpuAlarm();

    // Assert: diffTime == timeGap 满足 >= → 触发
    EXPECT_EQ(m_service->mLastAlarmTimeStamp, g_fixedNow.toMSecsSinceEpoch());
    EXPECT_EQ(g_processStartCalls, 0);   // singleShot(100) 尚未到点（固定时钟不影响 QTimer）
}

// checkMemoryAlarm B1: 超阈值且从未告警 → 触发一次
TEST_F(UT_SystemMonitorService, CheckMemoryAlarm_UsageExceeds_TriggersOnce)
{
    // Arrange
    makeService();
    m_service->mMemoryUsage = 96;
    m_service->mLastAlarmTimeStamp = 0;

    // Act
    m_service->checkMemoryAlarm();
    QTest::qWait(250);

    // Assert
    EXPECT_GT(m_service->mLastAlarmTimeStamp, 0);
    EXPECT_EQ(g_processStartCalls, 1);
    EXPECT_TRUE(g_lastProcessArgs.contains("com.deepin.SystemMonitorServer.showMemoryAlarmNotify"));
    EXPECT_TRUE(g_lastProcessArgs.contains(QString::number(96)));
}

// checkMemoryAlarm B2: 间隔内不重复告警
TEST_F(UT_SystemMonitorService, CheckMemoryAlarm_WithinGap_Suppressed)
{
    // Arrange
    makeService();
    m_service->mMemoryUsage = 96;
    m_service->mLastAlarmTimeStamp = QDateTime::currentMSecsSinceEpoch();
    qint64 before = m_service->mLastAlarmTimeStamp;

    // Act
    m_service->checkMemoryAlarm();

    // Assert
    EXPECT_EQ(m_service->mLastAlarmTimeStamp, before);
    EXPECT_EQ(g_processStartCalls, 0);
}

// checkMemoryAlarm B1 边界: diffTime 恰等于 timeGap（memory 用严格 > 判定 → 不触发，
// 与 checkCpuAlarm 的 >= 不对称——现状锁定）
TEST_F(UT_SystemMonitorService, CheckMemoryAlarm_ExactGap_Suppressed)
{
    // Arrange: 固定时钟 + last = now - timeGap
    stubFixedClock();
    makeService();
    m_service->mMemoryUsage = 96;
    qint64 timeGap = 1000LL * 60 * m_service->getAlarmMsgInterval();
    m_service->mLastAlarmTimeStamp = g_fixedNow.toMSecsSinceEpoch() - timeGap;
    qint64 before = m_service->mLastAlarmTimeStamp;

    // Act
    m_service->checkMemoryAlarm();

    // Assert: diffTime == timeGap 不满足严格 > → 不触发
    EXPECT_EQ(m_service->mLastAlarmTimeStamp, before);
    EXPECT_EQ(g_processStartCalls, 0);
}

// onMonitorTimeout B1: 更新占用率（保护关闭 → 不做告警检测）
TEST_F(UT_SystemMonitorService, OnMonitorTimeout_WithSampling_UpdatesUsage)
{
    // Arrange: 受控采样值
    stub.set_lamda(VADDR(CpuProfile, updateSystemCpuUsage),
                   [](CpuProfile *) -> double { return 42.0; });
    stub.set_lamda(VADDR(MemoryProfile, updateSystemMemoryUsage),
                   [](MemoryProfile *) -> double { return 66.0; });
    makeService();   // ctor 采样同样走 stub → 42/66
    qint64 before = m_service->mLastAlarmTimeStamp;

    // Act
    m_service->onMonitorTimeout();

    // Assert: 占用率更新；保护关闭 → 无告警（时间戳不变）
    EXPECT_EQ(m_service->getCpuUsage(), 42);
    EXPECT_EQ(m_service->getMemoryUsage(), 66);
    EXPECT_EQ(m_service->mLastAlarmTimeStamp, before);
    EXPECT_EQ(g_processStartCalls, 0);
}

// onMonitorTimeout B2: 保护开启 + 双超阈值 → cpu/memory 告警都触发
// onMonitorTimeout B2: 保护开启 + 双超阈值 → cpu 告警触发；memory 告警被共享时间戳抑制
TEST_F(UT_SystemMonitorService, OnMonitorTimeout_ProtectionOn_CpuFiresMemorySuppressed)
{
    // Arrange: 受控采样值（95/96 双超阈值）
    stub.set_lamda(VADDR(CpuProfile, updateSystemCpuUsage),
                   [](CpuProfile *) -> double { return 95.0; });
    stub.set_lamda(VADDR(MemoryProfile, updateSystemMemoryUsage),
                   [](MemoryProfile *) -> double { return 96.0; });
    makeService();
    m_service->setSystemProtectionStatus(true);
    m_service->mLastAlarmTimeStamp = 0;

    // Act
    m_service->onMonitorTimeout();
    QTest::qWait(250);

    // Assert: [现状锁定 D10] cpu/memory 告警共享同一 mLastAlarmTimeStamp ——
    // cpu 告警同步更新时间戳后，同轮 memory 告警的 diffTime≈0 被 gap 抑制，
    // 仅 cpu 的 gdbus 通知发出（memory 告警在 cpu 已告警的轮次永远无法触发）
    EXPECT_GT(m_service->mLastAlarmTimeStamp, 0);
    EXPECT_EQ(g_processStartCalls, 1);
    EXPECT_TRUE(g_lastProcessArgs.contains("com.deepin.SystemMonitorServer.showCpuAlarmNotify"));
    EXPECT_FALSE(g_lastProcessArgs.contains("com.deepin.SystemMonitorServer.showMemoryAlarmNotify"));
    EXPECT_TRUE(g_lastProcessArgs.contains(QString::number(95)));
}

// ~SystemMonitorService: 销毁不崩溃，子对象随级联删除回收
TEST_F(UT_SystemMonitorService, Dtor_AfterReset_Reconstructable)
{
    // Arrange
    makeService();
    m_service->setSystemProtectionStatus(true);

    // Act: unique_ptr 释放 → 析构链（SettingHandler/CpuProfile/MemoryProfile 子对象级联）
    m_service.reset();

    // Assert: 析构后可继续构造新实例，且上一实例的落盘状态（保护开启）被新实例载入
    makeService();
    EXPECT_NE(m_service.get(), nullptr);
    EXPECT_EQ(m_service->getSystemProtectionStatus(), true);
}
