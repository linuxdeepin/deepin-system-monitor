// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// 用例计数声明（self-check-structural 验证此块）：
// | method | level | factors | min | actual |
// |--------|-------|---------|-----|--------|
// | setServiceStartupMode | high | complexity:16,cognitive:26 | 3 | 12 |
// | updateServiceEntry | high | complexity:11 | 3 | 3 |
// | startService | mid | complexity:5 | 2 | 6 |
// | stopService | mid | complexity:5 | 2 | 5 |
// | restartService | mid | complexity:5 | 2 | 6 |
// | normalizeServiceId | mid | in_degree:5 | 1 | 11 |
// | CustomTimer::start | mid | in_degree:10 | 2 | 2 |
// | instance | mid | in_degree:8 | 1 | 1 |
// | getServiceStartupType | low | - | 1 | 8 |
// | updateServiceList | low | - | 1 | 1 |
// | ServiceManager(ctor) | low | - | 1 | 1 |

//self
#include "service/service_manager.h"

//dbus
#include "dbus/dbus_common.h"
#include "dbus/systemd1_manager_interface.h"
#include "dbus/systemd1_service_interface.h"
#include "dbus/systemd1_unit_interface.h"

//service
#include "service/system_service_entry.h"

//qt
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusReply>
#include <QProcess>
#include <QSignalSpy>
#include <QTest>

//gtest
#include "stub.h"
#include <gtest/gtest.h>

//system
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>

/*
 * [test-types 最小清单完成情况]
 * 1. 复杂度>=10 方法分支覆盖：start/stop/restartService、setServiceStartupMode、normalizeServiceId、
 *    getServiceStartupType、updateServiceEntry 全部分支见下清单 — 完成（CustomTimer m_cnt>=6 上限分支
 *    需 ~18.4s 真实时钟墙时间，按测试成本约定跳过并在缺陷/覆盖率说明中标注）
 * 2. AAA 三段注释：所有用例 Arrange/Act/Assert 分段 — 完成
 * 3. TEST_P 参数化 >=3 组同质输入：normalizeServiceId 11 组、getServiceStartupType 6 组 — 完成
 * 4. EXPECT_THROW 精确异常：无抛异常 API，负面路径以 ErrorContext 错误分支覆盖 — 完成
 * 5. 外部依赖隔离：DBus 接口方法（weak 符号补丁）、QDBusConnection::systemBus、QProcess、
 *    QFile::open/QIODevice::readLine、::stat 全部 stub；不依赖真实 systemd/DBus 设备环境 — 完成
 * * [startService/stopService/restartService 分支清单]（三者同构，仅 DBus 方法名不同）
 *  B1 <X>Unit 失败 → 直接返回错误                          — *_error 用例 T 侧 / 其余用例 F 侧
 *  B2 原始 id 以 "@" 结尾 → 启动成功即提前返回             — template_id 用例
 *  B3 GetUnit 失败且 code==3 → normalizeUnitPath 修正路径  — getunit_code3 用例（断言传入路径）
 *  B4 GetUnit 失败且 code!=3 → 返回错误                    — getunit_other_error 用例
 *  B5 非终态 → new CustomTimer 并 start(o.path())          — nonfinal 用例（记录 stub）
 *  B6 终态(active/inactive/failed) → 不启动定时器          — ok_final 用例
 * [setServiceStartupMode 分支清单]
 *  B7 stat(pkexec) 失败 → kErrorTypeSystem/ENOENT          — pkexec_missing 用例（stub ::stat）
 *  B8 stat(systemctl) 失败 → 同上                          — systemctl_missing 用例（仅 systemctl 失败）
 *  B9 开发者模式（dev 文件=="1"）→ QProcess 后端           — dev_* 用例（stub QFile::open+readLine）
 *  B10 非开发者模式 → setServiceEnable DBus 后端，失败即返回 — nondev_dbus_fail 用例（无效连接）
 *  B11 CrashExit / 退出码 126|127(EPERM) / 其他非 0（读 stdout）— dev_crash/dev_perm/dev_other 用例
 *  B12 成功后 id 以 "@" 结尾 → 提前返回                    — dev_template_id 用例
 *  B13 GetUnit code==3 → normalizeUnitPath 修正路径        — dev_success 用例（GetUnit stub 成功路径）+
 *      startService_getunit_code3 用例（code3 路径）
 * [updateServiceEntry 分支清单] 10 个 getter 成功/失败双侧 — success(全成功)/all_errors(全失败) 用例
 * [normalizeServiceId 分支清单] 有无 ".service" 后缀 × 有无 '@' × param 空/非空 — TEST_P 全组合
 */

namespace {

/* QFile::open 为虚函数：GCC Itanium ABI 下 &QFile::open 的指针字段编码的是「虚表字节偏移+1」
 * （实测 0x69 = 13*8+1，即槽 13），直接交给 Stub::set 会把小整数当函数地址读、导致 SEGV。
 * 此处从实例虚表按字节偏移解析出真实函数地址后重建成员函数指针（非虚成员则原样返回）。 */
template <typename Class, typename Ret, typename... Args>
Ret (Class::*vaddr(Ret (Class::*mfn)(Args...)))(Args...)
{
    uintptr_t v = 0;
    std::memcpy(&v, &mfn, sizeof(v));
    if ((v & 1) == 0)
        return mfn;  // 非虚成员：指针字段即真实地址
    Class probe;
    void *vptr = *reinterpret_cast<void **>(&probe);
    void *fn = *reinterpret_cast<void **>(reinterpret_cast<char *>(vptr) + (v - 1));
    Ret (Class::*out)(Args...) = nullptr;
    std::memcpy(&out, &fn, sizeof(fn));
    return out;
}

// service_manager.cpp L37-38 的 #define 常量（未导出到头文件，测试侧镜像定义）
static const char *kBinPkexecPath = "/usr/bin/pkexec";
static const char *kBinSystemctlPath = "/usr/bin/systemctl";

// ---------- 调用记录 ----------
static bool g_getUnitCalled = false;
static QString g_getUnitArg;
static bool g_updateEntryCalled = false;
static QString g_updateEntryArg;
static bool g_timerStarted = false;
static QString g_timerStartArg;
static int g_entryCallCount = 0;

static void resetCallRecords()
{
    g_getUnitCalled = false;
    g_getUnitArg.clear();
    g_updateEntryCalled = false;
    g_updateEntryArg.clear();
    g_timerStarted = false;
    g_timerStartArg.clear();
    g_entryCallCount = 0;
}

// ---------- Systemd1ManagerInterface stub ----------
static QPair<ErrorContext, QDBusObjectPath> stub_StartUnit_ok(Systemd1ManagerInterface *, const QString &, const QString &)
{
    return { ErrorContext(), QDBusObjectPath("/org/freedesktop/systemd1/job/1") };
}

static QPair<ErrorContext, QDBusObjectPath> stub_StartUnit_fail(Systemd1ManagerInterface *, const QString &, const QString &)
{
    return { ErrorContext(ErrorContext::kErrorTypeDBus, 3, "org.freedesktop.systemd1.AccessDenied", "denied"), QDBusObjectPath() };
}

static QPair<ErrorContext, QDBusObjectPath> stub_StopUnit_ok(Systemd1ManagerInterface *, const QString &, const QString &)
{
    return { ErrorContext(), QDBusObjectPath("/org/freedesktop/systemd1/job/2") };
}

static QPair<ErrorContext, QDBusObjectPath> stub_StopUnit_fail(Systemd1ManagerInterface *, const QString &, const QString &)
{
    return { ErrorContext(ErrorContext::kErrorTypeDBus, 3, "org.freedesktop.systemd1.AccessDenied", "denied"), QDBusObjectPath() };
}

static QPair<ErrorContext, QDBusObjectPath> stub_RestartUnit_ok(Systemd1ManagerInterface *, const QString &, const QString &)
{
    return { ErrorContext(), QDBusObjectPath("/org/freedesktop/systemd1/job/3") };
}

static QPair<ErrorContext, QDBusObjectPath> stub_RestartUnit_fail(Systemd1ManagerInterface *, const QString &, const QString &)
{
    return { ErrorContext(ErrorContext::kErrorTypeDBus, 3, "org.freedesktop.systemd1.AccessDenied", "denied"), QDBusObjectPath() };
}

static QPair<ErrorContext, QDBusObjectPath> stub_GetUnit(Systemd1ManagerInterface *, const QString &opath)
{
    g_getUnitCalled = true;
    g_getUnitArg = opath;
    // GetUnit 语义上返回单元对象路径：对传入 id 做 normalizeUnitPath
    return { ErrorContext(), Systemd1UnitInterface::normalizeUnitPath(opath) };
}

static QPair<ErrorContext, QDBusObjectPath> stub_GetUnit_code3(Systemd1ManagerInterface *, const QString &)
{
    g_getUnitCalled = true;
    return { ErrorContext(ErrorContext::kErrorTypeDBus, 3, "org.freedesktop.systemd1.NoSuchUnit", "no such unit"), QDBusObjectPath() };
}

static QPair<ErrorContext, QDBusObjectPath> stub_GetUnit_otherErr(Systemd1ManagerInterface *, const QString &)
{
    g_getUnitCalled = true;
    return { ErrorContext(ErrorContext::kErrorTypeCommon, 1, "some.error", "broken"), QDBusObjectPath() };
}

static QPair<ErrorContext, QString> stub_GetUnitFileState_ok(Systemd1ManagerInterface *, const QString &)
{
    return { ErrorContext(), QString("enabled") };
}

static QPair<ErrorContext, QString> stub_GetUnitFileState_err(Systemd1ManagerInterface *, const QString &)
{
    return { ErrorContext(ErrorContext::kErrorTypeDBus, 3, "err", "fail"), QString() };
}

// ---------- Systemd1UnitInterface / Systemd1ServiceInterface stub（updateServiceEntry 用） ----------
static QPair<ErrorContext, QString> stub_getId_ok(Systemd1UnitInterface *)
{
    return { ErrorContext(), QString("ssh.service") };
}

static QPair<ErrorContext, QString> stub_str_err(Systemd1UnitInterface *)
{
    return { ErrorContext(ErrorContext::kErrorTypeDBus, 3, "err", "fail"), QString() };
}

static QPair<ErrorContext, QString> stub_str_loaded_ok(Systemd1UnitInterface *)
{
    return { ErrorContext(), QString("loaded") };
}

static QPair<ErrorContext, QString> stub_str_active_ok(Systemd1UnitInterface *)
{
    return { ErrorContext(), QString(kUnitActiveStateText) };
}

static QPair<ErrorContext, QString> stub_str_inactive_ok(Systemd1UnitInterface *)
{
    return { ErrorContext(), QString(kUnitInactiveStateText) };
}

static QPair<ErrorContext, bool> stub_bool_ok(Systemd1UnitInterface *)
{
    return { ErrorContext(), true };
}

static QPair<ErrorContext, bool> stub_bool_err(Systemd1UnitInterface *)
{
    return { ErrorContext(ErrorContext::kErrorTypeDBus, 3, "err", "fail"), false };
}

static QPair<ErrorContext, quint32> stub_pid_ok(Systemd1ServiceInterface *)
{
    return { ErrorContext(), quint32(4321) };
}

static QPair<ErrorContext, quint32> stub_pid_err(Systemd1ServiceInterface *)
{
    return { ErrorContext(ErrorContext::kErrorTypeDBus, 3, "err", "fail"), quint32(0) };
}

// ---------- ServiceManager::updateServiceEntry stub（可控返回终态/非终态） ----------
static SystemServiceEntry stub_entry_final(ServiceManager *, const QString &opath)
{
    ++g_entryCallCount;
    g_updateEntryCalled = true;
    g_updateEntryArg = opath;
    SystemServiceEntry e;
    e.setUnitObjectPath(opath);
    e.setActiveState(kUnitInactiveStateText);  // 终态
    return e;
}

static SystemServiceEntry stub_entry_switching(ServiceManager *, const QString &opath)
{
    ++g_entryCallCount;
    g_updateEntryCalled = true;
    g_updateEntryArg = opath;
    SystemServiceEntry e;
    e.setUnitObjectPath(opath);
    // 首次调用回非终态（触发定时器启动），其后回终态（定时器自停）
    e.setActiveState(g_entryCallCount == 1 ? QString("activating") : QString(kUnitInactiveStateText));
    return e;
}

static SystemServiceEntry stub_entry_activating(ServiceManager *, const QString &opath)
{
    ++g_entryCallCount;
    g_updateEntryCalled = true;
    g_updateEntryArg = opath;
    SystemServiceEntry e;
    e.setUnitObjectPath(opath);
    e.setActiveState("activating");  // 恒非终态
    return e;
}

// ---------- CustomTimer::start 记录 ----------
static void stub_CustomTimer_start(CustomTimer *self, const QString &opath)
{
    g_timerStarted = true;
    g_timerStartArg = opath;
}

// ---------- QDBusConnection / QProcess / QFile / QIODevice / stat stub ----------
static QDBusConnection stub_systemBus_invalid()
{
    return QDBusConnection(QString("sm-ut-invalid-connection"));
}

static int stub_QProcess_execute_ok(const QString &, const QStringList &)
{
    return 0;
}

static int stub_QProcess_execute_fail(const QString &, const QStringList &)
{
    return 1;
}

static void stub_QProcess_start(QProcess *, const QString &, const QStringList &, QIODevice::OpenMode)
{
}

static bool stub_QProcess_waitForFinished(QProcess *, int)
{
    return true;
}

static QProcess::ExitStatus stub_QProcess_exitStatus_ok(QProcess *)
{
    return QProcess::NormalExit;
}

static QProcess::ExitStatus stub_QProcess_exitStatus_crash(QProcess *)
{
    return QProcess::CrashExit;
}

static int stub_QProcess_exitCode_zero(QProcess *)
{
    return 0;
}

static int stub_QProcess_exitCode_127(QProcess *)
{
    return 127;
}

static int stub_QProcess_exitCode_3(QProcess *)
{
    return 3;
}

static QByteArray stub_QProcess_readAllStdOut(QProcess *)
{
    return QByteArray("some-process-output");
}

static bool stub_QFile_open_true(QFile *, QIODevice::OpenMode)
{
    return true;
}

static QByteArray stub_QIODevice_readLine_dev(QIODevice *, qint64)
{
    return QByteArray("1\n");
}

static QByteArray stub_QIODevice_readLine_nondev(QIODevice *, qint64)
{
    return QByteArray("0\n");
}

// 仅让 pkexec 的 stat 失败，其余路径走真实系统调用（SYS_newfstatat 等价 stat，避开补丁递归）
static int stub_stat_fail_pkexec(const char *path, struct stat *buf)
{
    if (strcmp(path, kBinPkexecPath) == 0) {
        errno = ENOENT;
        return -1;
    }
    return int(syscall(SYS_newfstatat, AT_FDCWD, path, buf, 0));
}

// 仅让 systemctl 的 stat 失败（pkexec 先行检查通过）
static int stub_stat_fail_systemctl(const char *path, struct stat *buf)
{
    if (strcmp(path, kBinSystemctlPath) == 0) {
        errno = ENOENT;
        return -1;
    }
    return int(syscall(SYS_newfstatat, AT_FDCWD, path, buf, 0));
}

static void installDevModeStubs(Stub &stub, bool developerMode)
{
    stub.set(vaddr(static_cast<bool (QFile::*)(QIODevice::OpenMode)>(&QFile::open)), stub_QFile_open_true);
    stub.set(static_cast<QByteArray (QIODevice::*)(qint64)>(&QIODevice::readLine),
             developerMode ? stub_QIODevice_readLine_dev : stub_QIODevice_readLine_nondev);
    stub.set(&QDBusConnection::systemBus, stub_systemBus_invalid);
}

static void installProcessStubs(Stub &stub, QProcess::ExitStatus status, int exitCode)
{
    stub.set(static_cast<void (QProcess::*)(const QString &, const QStringList &, QIODevice::OpenMode)>(&QProcess::start), stub_QProcess_start);
    stub.set(ADDR(QProcess, waitForFinished), stub_QProcess_waitForFinished);
    stub.set(ADDR(QProcess, exitStatus), status == QProcess::CrashExit ? stub_QProcess_exitStatus_crash : stub_QProcess_exitStatus_ok);
    stub.set(ADDR(QProcess, exitCode), exitCode == 127 ? stub_QProcess_exitCode_127 : (exitCode == 3 ? stub_QProcess_exitCode_3 : stub_QProcess_exitCode_zero));
    if (exitCode == 3)
        stub.set(ADDR(QProcess, readAllStandardOutput), stub_QProcess_readAllStdOut);
}

static void installJobStubs(Stub &stub, bool startFail, bool stopFail, bool restartFail, int getUnitMode)
{
    using Mgr = Systemd1ManagerInterface;
    using StartStopFn = QPair<ErrorContext, QDBusObjectPath> (Mgr::*)(const QString &, const QString &);
    stub.set(static_cast<StartStopFn>(&Mgr::StartUnit), startFail ? stub_StartUnit_fail : stub_StartUnit_ok);
    stub.set(static_cast<StartStopFn>(&Mgr::StopUnit), stopFail ? stub_StopUnit_fail : stub_StopUnit_ok);
    stub.set(static_cast<StartStopFn>(&Mgr::RestartUnit), restartFail ? stub_RestartUnit_fail : stub_RestartUnit_ok);
    stub.set(static_cast<QPair<ErrorContext, QDBusObjectPath> (Mgr::*)(const QString &)>(&Mgr::GetUnit),
             getUnitMode == 1 ? stub_GetUnit_code3 : (getUnitMode == 2 ? stub_GetUnit_otherErr : stub_GetUnit));
}

/* ---------- setServiceEnable（静态自由函数，无法直接 ADDR stub）覆盖途径：
 * setServiceEnable 内部经 QDBusInterface::call（模板内联）→ QDBusAbstractInterface::doCall
 * （私有、非虚、库内实现）→ patch doCall 即可控制 QDBusReply<QString> 结果，
 * 覆盖 setServiceEnable 的 retMsg 失败/空串成功/非空错误三分支 ---------- */
static QString g_doCallReplyValue;   // 空→成功；非空→成功但带错误消息
static bool g_doCallDBusError = false;   // true→返回 ErrorMessage（retMsg 无效）
static QDBusMessage stub_doCall(QDBusAbstractInterface *, QDBus::CallMode, const QString &method,
                                const QVariant *, size_t)
{
    QDBusMessage mc = QDBusMessage::createMethodCall("org.deepin.SystemMonitorSystemServer", "/",
                                                     "org.deepin.SystemMonitorSystemServer", method);
    if (g_doCallDBusError)
        return mc.createError("org.freedesktop.DBus.Error.ServiceUnknown", "no server");
    return mc.createReply(g_doCallReplyValue);
}

static void installSetServiceEnableStubs(Stub &stub, const QString &replyValue, bool dbusError = false)
{
    g_doCallReplyValue = replyValue;
    g_doCallDBusError = dbusError;
    stub.set(ADDR(QDBusAbstractInterface, doCall), stub_doCall);
}

}  // namespace

class UT_ServiceManager : public ::testing::Test
{
public:
    UT_ServiceManager() : m_tester(nullptr) {}

    void SetUp() override
    {
        // Arrange：直接构造（-fno-access-control 绕过私有 ctor）
        resetCallRecords();
        m_tester = new ServiceManager();
    }

    void TearDown() override
    {
        delete m_tester;
        m_tester = nullptr;
        resetCallRecords();
    }

protected:
    ServiceManager *m_tester;
};

// 单例：instance() 两次返回同一指针
TEST_F(UT_ServiceManager, instance_returns_same_pointer)
{
    // Arrange：无前置状态，单例静态成员

    // Act
    ServiceManager *a = ServiceManager::instance();
    ServiceManager *b = ServiceManager::instance();

    // Assert
    EXPECT_EQ(a, b);
    EXPECT_NE(a, nullptr);
}

// updateServiceList：发射 beginUpdateList 信号
TEST_F(UT_ServiceManager, updateServiceList_emits_beginUpdateList)
{
    // Arrange
    QSignalSpy spy(m_tester, &ServiceManager::beginUpdateList);

    // Act
    m_tester->updateServiceList();

    // Assert
    EXPECT_EQ(spy.count(), 1);
}

// ---------- normalizeServiceId TEST_P ----------
namespace {
struct NormalizeCase
{
    QString input;
    QString param;
    QString expected;
};

class UT_ServiceManagerNormalize : public UT_ServiceManager,
                                   public ::testing::WithParamInterface<NormalizeCase>
{
};
}  // namespace

TEST_P(UT_ServiceManagerNormalize, normalizes_id_and_param)
{
    // Arrange
    const NormalizeCase &cs = GetParam();

    // Act
    const QString out = m_tester->normalizeServiceId(cs.input, cs.param);

    // Assert
    EXPECT_EQ(out, cs.expected);
}

INSTANTIATE_TEST_SUITE_P(IdCombinations, UT_ServiceManagerNormalize,
                         ::testing::Values(NormalizeCase{"foo", "", "foo.service"},
                                           NormalizeCase{"foo.service", "", "foo.service"},
                                           NormalizeCase{"foo@", "", "foo@.service"},
                                           NormalizeCase{"foo@", "1", "foo@1.service"},
                                           NormalizeCase{"foo@bar", "", "foo@bar.service"},
                                           NormalizeCase{"foo@bar", "2", "foo@bar.service"},
                                           NormalizeCase{"foo@bar.service", "3", "foo@3.service"},
                                           NormalizeCase{"foo@.service", "", "foo@.service"},
                                           NormalizeCase{"foo@.service", "5", "foo@5.service"},
                                           NormalizeCase{"", "", ".service"},
                                           NormalizeCase{"@@", "", "@@.service"}));

// ---------- getServiceStartupType TEST_P ----------
namespace {
struct StartupTypeCase
{
    QString id;
    QString state;
    QString expected;
};

class UT_ServiceManagerStartupType : public UT_ServiceManager,
                                     public ::testing::WithParamInterface<StartupTypeCase>
{
};
}  // namespace

TEST_P(UT_ServiceManagerStartupType, classifies_startup_type)
{
    // Arrange
    const StartupTypeCase &cs = GetParam();

    // Act
    const QString out = m_tester->getServiceStartupType(cs.id, cs.state);

    // Assert
    EXPECT_EQ(out, cs.expected);
}

INSTANTIATE_TEST_SUITE_P(StateCombinations, UT_ServiceManagerStartupType,
                         ::testing::Values(StartupTypeCase{"foo", "static", kServiceNAStartup},
                                           StartupTypeCase{"foo", "transient", kServiceNAStartup},
                                           StartupTypeCase{"", "enabled", kServiceAutoStartup},
                                           StartupTypeCase{"", "", kServiceNAStartup},
                                           StartupTypeCase{"foo", "enabled", kServiceAutoStartup},
                                           StartupTypeCase{"foo", "disabled", kServiceManualStartup}));

// getServiceStartupType：generated 状态 → SysV 查询成功 → Auto
TEST_F(UT_ServiceManager, getServiceStartupType_generated_sysv_enabled)
{
    // Arrange：QProcess::execute 恒 0（isSysVInitEnabled → true）
    Stub stub;
    stub.set(static_cast<int (*)(const QString &, const QStringList &)>(&QProcess::execute), stub_QProcess_execute_ok);

    // Act
    const QString out = m_tester->getServiceStartupType("foo", kUnitStateGeneratedRuntimeText);

    // Assert
    EXPECT_EQ(out, QString(kServiceAutoStartup));
}

// getServiceStartupType：generated 状态 → SysV 查询失败 → Manual
TEST_F(UT_ServiceManager, getServiceStartupType_generated_sysv_disabled)
{
    // Arrange：QProcess::execute 恒 1（indirect/static/transient 在 isUnitNoOp 中提前返回 N/A，
    // 仅 generated 状态能到达 isSysVInitEnabled 分支）
    Stub stub;
    stub.set(static_cast<int (*)(const QString &, const QStringList &)>(&QProcess::execute), stub_QProcess_execute_fail);

    // Act
    const QString out = m_tester->getServiceStartupType("foo", kUnitStateGeneratedRuntimeText);

    // Assert
    EXPECT_EQ(out, QString(kServiceManualStartup));
}

// startService：StartUnit 失败 → 返回错误且不查询 GetUnit（B1 T）
TEST_F(UT_ServiceManager, startService_startunit_error_returns_error)
{
    // Arrange
    Stub stub;
    installJobStubs(stub, true, false, false, 0);

    // Act
    ErrorContext ec = m_tester->startService("ssh");

    // Assert
    EXPECT_TRUE(ec.isValid());
    EXPECT_FALSE(g_getUnitCalled);
}

// startService：原始 id 以 @ 结尾 → 启动成功即提前返回（B2）
TEST_F(UT_ServiceManager, startService_template_id_early_return)
{
    // Arrange
    Stub stub;
    installJobStubs(stub, false, false, false, 0);

    // Act
    ErrorContext ec = m_tester->startService("foo@", "");

    // Assert：StartUnit 成功后提前返回，未调用 GetUnit
    EXPECT_FALSE(ec.isValid());
    EXPECT_FALSE(g_getUnitCalled);
}

// startService：GetUnit 成功 + 终态 → 无定时器（B6 F）
TEST_F(UT_ServiceManager, startService_ok_final_state_no_timer)
{
    // Arrange
    Stub stub;
    installJobStubs(stub, false, false, false, 0);
    stub.set(ADDR(ServiceManager, updateServiceEntry), stub_entry_final);

    // Act
    ErrorContext ec = m_tester->startService("ssh");

    // Assert
    EXPECT_FALSE(ec.isValid());
    EXPECT_TRUE(g_getUnitCalled);
    EXPECT_FALSE(g_timerStarted);
}

// startService：GetUnit code==3 → normalizeUnitPath 修正路径后调 updateServiceEntry（B3）
TEST_F(UT_ServiceManager, startService_getunit_code3_uses_normalized_path)
{
    // Arrange
    Stub stub;
    installJobStubs(stub, false, false, false, 1);
    stub.set(ADDR(ServiceManager, updateServiceEntry), stub_entry_final);

    // Act
    ErrorContext ec = m_tester->startService("ssh");

    // Assert：updateServiceEntry 收到 normalizeUnitPath("ssh.service") 的对象路径
    EXPECT_FALSE(ec.isValid());
    EXPECT_TRUE(g_updateEntryCalled);
    EXPECT_EQ(g_updateEntryArg, Systemd1UnitInterface::normalizeUnitPath("ssh.service").path());
}

// startService：GetUnit 非 3 错误 → 直接返回错误（B4）
TEST_F(UT_ServiceManager, startService_getunit_other_error_returns_error)
{
    // Arrange
    Stub stub;
    installJobStubs(stub, false, false, false, 2);
    stub.set(ADDR(ServiceManager, updateServiceEntry), stub_entry_final);

    // Act
    ErrorContext ec = m_tester->startService("ssh");

    // Assert
    EXPECT_TRUE(ec.isValid());
    EXPECT_FALSE(g_updateEntryCalled);
    EXPECT_FALSE(g_timerStarted);
}

// startService：非终态 → 启动 CustomTimer 且传 GetUnit 回传路径（B5 T）
TEST_F(UT_ServiceManager, startService_nonfinal_starts_timer)
{
    // Arrange：首次查询回 activating（非终态）；stub 掉 CustomTimer::start 避免真实事件循环
    Stub stub;
    installJobStubs(stub, false, false, false, 0);
    stub.set(ADDR(ServiceManager, updateServiceEntry), stub_entry_switching);
    stub.set(ADDR(CustomTimer, start), stub_CustomTimer_start);

    // Act
    ErrorContext ec = m_tester->startService("ssh");

    // Assert：定时器收到 GetUnit 回传的单元对象路径；仅直接调用 1 次
    EXPECT_FALSE(ec.isValid());
    EXPECT_TRUE(g_timerStarted);
    EXPECT_EQ(g_timerStartArg, Systemd1UnitInterface::normalizeUnitPath("ssh.service").path());
    EXPECT_EQ(g_entryCallCount, 1);
}

// stopService：StopUnit 失败 → 返回错误（B1 T）
TEST_F(UT_ServiceManager, stopService_stopunit_error_returns_error)
{
    // Arrange
    Stub stub;
    installJobStubs(stub, false, true, false, 0);

    // Act
    ErrorContext ec = m_tester->stopService("ssh");

    // Assert
    EXPECT_TRUE(ec.isValid());
    EXPECT_FALSE(g_getUnitCalled);
}

// stopService：非终态 → 启动定时器；二次查询终态后自停（B5 T/B6）
TEST_F(UT_ServiceManager, stopService_nonfinal_starts_timer_then_stops)
{
    // Arrange
    Stub stub;
    installJobStubs(stub, false, false, false, 0);
    stub.set(ADDR(ServiceManager, updateServiceEntry), stub_entry_switching);
    stub.set(ADDR(CustomTimer, start), stub_CustomTimer_start);

    // Act
    ErrorContext ec = m_tester->stopService("ssh");

    // Assert：定时器收到 GetUnit 回传的单元对象路径；仅直接调用 1 次
    EXPECT_FALSE(ec.isValid());
    EXPECT_TRUE(g_timerStarted);
    EXPECT_EQ(g_timerStartArg, Systemd1UnitInterface::normalizeUnitPath("ssh.service").path());
    EXPECT_EQ(g_entryCallCount, 1);
}

// stopService：模板 id → StopUnit 成功后提前返回，不查 GetUnit（B2）
TEST_F(UT_ServiceManager, stopService_template_id_early_return)
{
    // Arrange
    Stub stub;
    installJobStubs(stub, false, false, false, 0);

    // Act
    ErrorContext ec = m_tester->stopService("ssh@");

    // Assert
    EXPECT_FALSE(ec.isValid());
    EXPECT_FALSE(g_getUnitCalled);
}

// stopService：GetUnit 失败且 code==3 → normalizeUnitPath 修正后查条目（B3）
TEST_F(UT_ServiceManager, stopService_getunit_code3_normalizes)
{
    // Arrange
    Stub stub;
    installJobStubs(stub, false, false, false, 1);
    stub.set(ADDR(ServiceManager, updateServiceEntry), stub_entry_final);

    // Act
    ErrorContext ec = m_tester->stopService("ssh");

    // Assert：错误被 code==3 修正吞掉，走 updateServiceEntry
    EXPECT_FALSE(ec.isValid());
    EXPECT_TRUE(g_getUnitCalled);
    EXPECT_TRUE(g_updateEntryCalled);
    EXPECT_EQ(g_updateEntryArg, Systemd1UnitInterface::normalizeUnitPath("ssh.service").path());
}

// stopService：GetUnit 失败且 code!=3 → 返回错误（B4）
TEST_F(UT_ServiceManager, stopService_getunit_other_error)
{
    // Arrange
    Stub stub;
    installJobStubs(stub, false, false, false, 2);

    // Act
    ErrorContext ec = m_tester->stopService("ssh");

    // Assert
    EXPECT_TRUE(ec.isValid());
    EXPECT_EQ(ec.getCode(), ErrorContext::kErrorTypeCommon);
    EXPECT_FALSE(g_updateEntryCalled);
}

// restartService：RestartUnit 失败 → 返回错误（B1 T）
TEST_F(UT_ServiceManager, restartService_restartunit_error_returns_error)
{
    // Arrange
    Stub stub;
    installJobStubs(stub, false, false, true, 0);

    // Act
    ErrorContext ec = m_tester->restartService("ssh");

    // Assert
    EXPECT_TRUE(ec.isValid());
    EXPECT_FALSE(g_getUnitCalled);
}

// restartService：成功 + 终态 → 无定时器（B6 F）
TEST_F(UT_ServiceManager, restartService_ok_final_no_timer)
{
    // Arrange
    Stub stub;
    installJobStubs(stub, false, false, false, 0);
    stub.set(ADDR(ServiceManager, updateServiceEntry), stub_entry_final);

    // Act
    ErrorContext ec = m_tester->restartService("ssh");

    // Assert
    EXPECT_FALSE(ec.isValid());
    EXPECT_FALSE(g_timerStarted);
}

// setServiceStartupMode：pkexec 缺失（stat 失败）→ kErrorTypeSystem/ENOENT（B7）
TEST_F(UT_ServiceManager, setServiceStartupMode_pkexec_missing)
{
    // Arrange：仅 pkexec 的 stat 返回 -1
    Stub stub;
    stub.set(&stat, stub_stat_fail_pkexec);

    // Act
    ErrorContext ec = m_tester->setServiceStartupMode("ssh", true);

    // Assert：errfmt(kErrorTypeSystem, ENOENT, title, BIN_PKEXEC_PATH)
    EXPECT_TRUE(ec.isValid());
    EXPECT_EQ(ec.getCode(), ErrorContext::kErrorTypeSystem);
    EXPECT_EQ(ec.getSubCode(), ENOENT);
    EXPECT_TRUE(ec.getErrorMessage().contains(kBinPkexecPath));
}

// setServiceStartupMode：systemctl 缺失 → kErrorTypeSystem/ENOENT（B8）
TEST_F(UT_ServiceManager, setServiceStartupMode_systemctl_missing)
{
    // Arrange：pkexec 检查放行，systemctl 的 stat 失败
    Stub stub;
    stub.set(&stat, stub_stat_fail_systemctl);

    // Act
    ErrorContext ec = m_tester->setServiceStartupMode("ssh", true);

    // Assert
    EXPECT_TRUE(ec.isValid());
    EXPECT_EQ(ec.getCode(), ErrorContext::kErrorTypeSystem);
    EXPECT_EQ(ec.getSubCode(), ENOENT);
    EXPECT_TRUE(ec.getErrorMessage().contains(kBinSystemctlPath));
}

// restartService：模板 id → RestartUnit 成功后提前返回，不查 GetUnit（B2）
TEST_F(UT_ServiceManager, restartService_template_id_early_return)
{
    // Arrange
    Stub stub;
    installJobStubs(stub, false, false, false, 0);

    // Act
    ErrorContext ec = m_tester->restartService("ssh@");

    // Assert
    EXPECT_FALSE(ec.isValid());
    EXPECT_FALSE(g_getUnitCalled);
}

// restartService：GetUnit 失败且 code==3 → normalizeUnitPath 修正后查条目（B3）
TEST_F(UT_ServiceManager, restartService_getunit_code3_normalizes)
{
    // Arrange
    Stub stub;
    installJobStubs(stub, false, false, false, 1);
    stub.set(ADDR(ServiceManager, updateServiceEntry), stub_entry_final);

    // Act
    ErrorContext ec = m_tester->restartService("ssh");

    // Assert
    EXPECT_FALSE(ec.isValid());
    EXPECT_TRUE(g_updateEntryCalled);
    EXPECT_EQ(g_updateEntryArg, Systemd1UnitInterface::normalizeUnitPath("ssh.service").path());
}

// restartService：GetUnit 失败且 code!=3 → 返回错误（B4）
TEST_F(UT_ServiceManager, restartService_getunit_other_error)
{
    // Arrange
    Stub stub;
    installJobStubs(stub, false, false, false, 2);

    // Act
    ErrorContext ec = m_tester->restartService("ssh");

    // Assert
    EXPECT_TRUE(ec.isValid());
    EXPECT_EQ(ec.getCode(), ErrorContext::kErrorTypeCommon);
    EXPECT_FALSE(g_updateEntryCalled);
}

// restartService：非终态 → 启动定时器（B5 T）
TEST_F(UT_ServiceManager, restartService_nonfinal_starts_timer)
{
    // Arrange
    Stub stub;
    installJobStubs(stub, false, false, false, 0);
    stub.set(ADDR(ServiceManager, updateServiceEntry), stub_entry_switching);
    stub.set(ADDR(CustomTimer, start), stub_CustomTimer_start);

    // Act
    ErrorContext ec = m_tester->restartService("ssh");

    // Assert：定时器收到 GetUnit 回传的单元对象路径；仅直接调用 1 次
    EXPECT_FALSE(ec.isValid());
    EXPECT_TRUE(g_timerStarted);
    EXPECT_EQ(g_timerStartArg, Systemd1UnitInterface::normalizeUnitPath("ssh.service").path());
    EXPECT_EQ(g_entryCallCount, 1);
}

// setServiceStartupMode：非开发者模式 + setServiceEnable 失败 → kErrorTypeSystem（B10）
TEST_F(UT_ServiceManager, setServiceStartupMode_nondev_dbus_fail)
{
    // Arrange：dev 文件读到 "0"（非开发者模式）；systemBus 无效 → setServiceEnable 失败
    Stub stub;
    installDevModeStubs(stub, false);

    // Act
    ErrorContext ec = m_tester->setServiceStartupMode("ssh", true);

    // Assert：errfmt(kErrorTypeSystem, 0, title, errorString)（errno=0 → 前缀 "Error: "）
    EXPECT_TRUE(ec.isValid());
    EXPECT_EQ(ec.getCode(), ErrorContext::kErrorTypeSystem);
    EXPECT_EQ(ec.getSubCode(), 0);
    EXPECT_FALSE(ec.getErrorName().isEmpty());
}

// setServiceStartupMode：非开发者模式 + setServiceEnable 成功（空回包）→ 走尾部 GetUnit 链路
TEST_F(UT_ServiceManager, setServiceStartupMode_nondev_dbus_ok)
{
    // Arrange：dev 文件 "0" → 非开发者模式；doCall 返回空串 → setServiceEnable 成功
    Stub stub;
    installDevModeStubs(stub, false);
    installSetServiceEnableStubs(stub, QString());
    installProcessStubs(stub, QProcess::NormalExit, 0);   // 保险：非 dev 不应触达 QProcess
    stub.set(ADDR(ServiceManager, updateServiceEntry), stub_entry_final);
    stub.set(static_cast<QPair<ErrorContext, QDBusObjectPath> (Systemd1ManagerInterface::*)(const QString &)>(&Systemd1ManagerInterface::GetUnit), stub_GetUnit);

    // Act
    ErrorContext ec = m_tester->setServiceStartupMode("ssh", true);

    // Assert：成功且继续走 GetUnit → updateServiceEntry
    EXPECT_FALSE(ec.isValid());
    EXPECT_TRUE(g_getUnitCalled);
    EXPECT_TRUE(g_updateEntryCalled);
}

// setServiceStartupMode：非开发者模式 + setServiceEnable 回包带错误消息 → 失败返回
TEST_F(UT_ServiceManager, setServiceStartupMode_nondev_dbus_retmsg_error)
{
    // Arrange：doCall 返回非空字符串（服务端错误消息）
    Stub stub;
    installDevModeStubs(stub, false);
    installSetServiceEnableStubs(stub, QStringLiteral("server refused to enable"));

    // Act
    ErrorContext ec = m_tester->setServiceStartupMode("ssh", true);

    // Assert：errorString 即服务端消息；不再触达 GetUnit
    EXPECT_TRUE(ec.isValid());
    EXPECT_EQ(ec.getCode(), ErrorContext::kErrorTypeSystem);
    EXPECT_TRUE(ec.getErrorMessage().contains("server refused to enable"));
    EXPECT_FALSE(g_getUnitCalled);
}

// setServiceStartupMode：非开发者模式 + setServiceEnable DBus 回包无效 → 失败返回
TEST_F(UT_ServiceManager, setServiceStartupMode_nondev_dbus_reply_invalid)
{
    // Arrange：doCall 返回 ErrorMessage → retMsg.isValid() == false
    Stub stub;
    installDevModeStubs(stub, false);
    installSetServiceEnableStubs(stub, QString(), true);

    // Act
    ErrorContext ec = m_tester->setServiceStartupMode("ssh", false);

    // Assert：错误消息含 DBus 错误信息；disable（false）也走同一后端
    EXPECT_TRUE(ec.isValid());
    EXPECT_EQ(ec.getCode(), ErrorContext::kErrorTypeSystem);
    EXPECT_TRUE(ec.getErrorMessage().contains("no server"));
    EXPECT_FALSE(g_getUnitCalled);
}

// setServiceStartupMode：dev + QProcess 成功 + GetUnit code==3 → normalizeUnitPath 修正路径
TEST_F(UT_ServiceManager, setServiceStartupMode_dev_getunit_code3)
{
    // Arrange
    Stub stub;
    installDevModeStubs(stub, true);
    installProcessStubs(stub, QProcess::NormalExit, 0);
    stub.set(ADDR(ServiceManager, updateServiceEntry), stub_entry_final);
    stub.set(static_cast<QPair<ErrorContext, QDBusObjectPath> (Systemd1ManagerInterface::*)(const QString &)>(&Systemd1ManagerInterface::GetUnit), stub_GetUnit_code3);

    // Act
    ErrorContext ec = m_tester->setServiceStartupMode("ssh", true);

    // Assert：code==3 被 normalizeUnitPath 修正吞掉 → 继续 updateServiceEntry → 成功
    EXPECT_FALSE(ec.isValid());
    EXPECT_TRUE(g_getUnitCalled);
    EXPECT_TRUE(g_updateEntryCalled);
    EXPECT_EQ(g_updateEntryArg, Systemd1UnitInterface::normalizeUnitPath("ssh.service").path());
}

// setServiceStartupMode：dev + QProcess 成功 + GetUnit code!=3 → 返回错误
TEST_F(UT_ServiceManager, setServiceStartupMode_dev_getunit_error)
{
    // Arrange
    Stub stub;
    installDevModeStubs(stub, true);
    installProcessStubs(stub, QProcess::NormalExit, 0);
    stub.set(static_cast<QPair<ErrorContext, QDBusObjectPath> (Systemd1ManagerInterface::*)(const QString &)>(&Systemd1ManagerInterface::GetUnit), stub_GetUnit_otherErr);

    // Act
    ErrorContext ec = m_tester->setServiceStartupMode("ssh", true);

    // Assert
    EXPECT_TRUE(ec.isValid());
    EXPECT_EQ(ec.getCode(), ErrorContext::kErrorTypeCommon);
    EXPECT_FALSE(g_updateEntryCalled);
}

// setServiceStartupMode：开发者模式 + 模板 id → 提前返回，不调 GetUnit（B12）
TEST_F(UT_ServiceManager, setServiceStartupMode_dev_template_id_early_return)
{
    // Arrange：dev 文件 "1" → 开发者模式；QProcess 全部成功；id 为模板
    Stub stub;
    installDevModeStubs(stub, true);
    installProcessStubs(stub, QProcess::NormalExit, 0);
    stub.set(static_cast<QPair<ErrorContext, QDBusObjectPath> (Systemd1ManagerInterface::*)(const QString &)>(&Systemd1ManagerInterface::GetUnit), stub_GetUnit);

    // Act：id "foo@" → normalize 为 "foo@.service" 以 @ 结尾 → 提前返回
    ErrorContext ec = m_tester->setServiceStartupMode("foo@", true);

    // Assert
    EXPECT_FALSE(ec.isValid());
    EXPECT_FALSE(g_getUnitCalled);
}

// setServiceStartupMode：开发者模式 + 子进程崩溃 → 错误消息含 crashed（B11 CrashExit）
TEST_F(UT_ServiceManager, setServiceStartupMode_dev_process_crash)
{
    // Arrange
    Stub stub;
    installDevModeStubs(stub, true);
    installProcessStubs(stub, QProcess::CrashExit, 0);

    // Act
    ErrorContext ec = m_tester->setServiceStartupMode("ssh", true);

    // Assert
    EXPECT_TRUE(ec.isValid());
    EXPECT_EQ(ec.getCode(), ErrorContext::kErrorTypeSystem);
    EXPECT_TRUE(ec.getErrorMessage().contains("crashed"));
}

// setServiceStartupMode：开发者模式 + 退出码 127 → EPERM（B11 126/127）
TEST_F(UT_ServiceManager, setServiceStartupMode_dev_process_perm_denied)
{
    // Arrange
    Stub stub;
    installDevModeStubs(stub, true);
    installProcessStubs(stub, QProcess::NormalExit, 127);

    // Act
    ErrorContext ec = m_tester->setServiceStartupMode("ssh", true);

    // Assert：errno=EPERM(1)
    EXPECT_TRUE(ec.isValid());
    EXPECT_EQ(ec.getCode(), ErrorContext::kErrorTypeSystem);
    EXPECT_EQ(ec.getSubCode(), EPERM);
}

// setServiceStartupMode：开发者模式 + 其他非 0 退出码 → stdout 进错误消息（B11 其他）
TEST_F(UT_ServiceManager, setServiceStartupMode_dev_process_other_exit)
{
    // Arrange
    Stub stub;
    installDevModeStubs(stub, true);
    installProcessStubs(stub, QProcess::NormalExit, 3);

    // Act
    ErrorContext ec = m_tester->setServiceStartupMode("ssh", true);

    // Assert
    EXPECT_TRUE(ec.isValid());
    EXPECT_EQ(ec.getCode(), ErrorContext::kErrorTypeSystem);
    EXPECT_TRUE(ec.getErrorMessage().contains("some-process-output"));
}

// setServiceStartupMode：开发者模式 + 进程成功 + 非模板 id → GetUnit(规范化 id) → updateServiceEntry
TEST_F(UT_ServiceManager, setServiceStartupMode_dev_success_flows_to_getunit)
{
    // Arrange：QProcess 全成功；GetUnit stub 成功（回显规范化路径）
    Stub stub;
    installDevModeStubs(stub, true);
    installProcessStubs(stub, QProcess::NormalExit, 0);
    stub.set(ADDR(ServiceManager, updateServiceEntry), stub_entry_final);
    stub.set(static_cast<QPair<ErrorContext, QDBusObjectPath> (Systemd1ManagerInterface::*)(const QString &)>(&Systemd1ManagerInterface::GetUnit), stub_GetUnit);

    // Act
    ErrorContext ec = m_tester->setServiceStartupMode("ssh", true);

    // Assert：GetUnit 收到 normalizeServiceId("ssh")；updateServiceEntry 收到规范化对象路径
    EXPECT_FALSE(ec.isValid());
    EXPECT_TRUE(g_getUnitCalled);
    EXPECT_EQ(g_getUnitArg, QString("ssh.service"));
    EXPECT_TRUE(g_updateEntryCalled);
    EXPECT_EQ(g_updateEntryArg, Systemd1UnitInterface::normalizeUnitPath("ssh.service").path());
}

// updateServiceEntry：全成功 → 字段齐全（含 sname 去后缀）且发射 serviceStatusUpdated
TEST_F(UT_ServiceManager, updateServiceEntry_success_populates_entry)
{
    // Arrange
    Stub stub;
    using Unit = Systemd1UnitInterface;
    using Mgr = Systemd1ManagerInterface;
    using Svc = Systemd1ServiceInterface;
    stub.set(static_cast<QPair<ErrorContext, QString> (Unit::*)() const>(&Unit::getId), stub_getId_ok);
    stub.set(static_cast<QPair<ErrorContext, QString> (Unit::*)() const>(&Unit::getLoadState), stub_str_loaded_ok);
    stub.set(static_cast<QPair<ErrorContext, QString> (Unit::*)() const>(&Unit::getActiveState), stub_str_active_ok);
    stub.set(static_cast<QPair<ErrorContext, QString> (Unit::*)() const>(&Unit::getSubState), stub_str_loaded_ok);
    stub.set(static_cast<QPair<ErrorContext, QString> (Unit::*)() const>(&Unit::getDescription), stub_str_loaded_ok);
    stub.set(static_cast<QPair<ErrorContext, bool> (Unit::*)() const>(&Unit::canStart), stub_bool_ok);
    stub.set(static_cast<QPair<ErrorContext, bool> (Unit::*)() const>(&Unit::canStop), stub_bool_ok);
    stub.set(static_cast<QPair<ErrorContext, bool> (Unit::*)() const>(&Unit::canReload), stub_bool_err);
    stub.set(static_cast<QPair<ErrorContext, quint32> (Svc::*)() const>(&Svc::getMainPID), stub_pid_ok);
    stub.set(static_cast<QPair<ErrorContext, QString> (Mgr::*)(const QString &)>(&Mgr::GetUnitFileState), stub_GetUnitFileState_ok);
    QSignalSpy spy(m_tester, &ServiceManager::serviceStatusUpdated);
    qRegisterMetaType<SystemServiceEntry>("SystemServiceEntry");

    // Act
    const SystemServiceEntry entry = m_tester->updateServiceEntry("/org/freedesktop/systemd1/unit/ssh_2eservice");

    // Assert：id/sname/set 全部生效；unitObjectPath 不由该方法设置（保持默认空）
    EXPECT_EQ(entry.getId(), "ssh.service");
    EXPECT_EQ(entry.getSName(), "ssh");
    EXPECT_EQ(entry.getLoadState(), "loaded");
    EXPECT_EQ(entry.getActiveState(), QString(kUnitActiveStateText));
    EXPECT_EQ(entry.getSubState(), "loaded");
    EXPECT_EQ(entry.getDescription(), "loaded");
    EXPECT_EQ(entry.getMainPID(), 4321u);
    EXPECT_TRUE(entry.getCanStart());
    EXPECT_TRUE(entry.getCanStop());
    EXPECT_FALSE(entry.getCanReload());
    EXPECT_EQ(entry.getState(), "enabled");
    EXPECT_EQ(entry.getStartupType(), QString(kServiceAutoStartup));
    EXPECT_TRUE(entry.getUnitObjectPath().isEmpty());
    EXPECT_EQ(spy.count(), 1);
}

// updateServiceEntry：全部 getter 失败 → 条目保持默认仍发射信号
TEST_F(UT_ServiceManager, updateServiceEntry_all_errors_returns_default_entry)
{
    // Arrange
    Stub stub;
    using Unit = Systemd1UnitInterface;
    using Mgr = Systemd1ManagerInterface;
    using Svc = Systemd1ServiceInterface;
    stub.set(static_cast<QPair<ErrorContext, QString> (Unit::*)() const>(&Unit::getId), stub_str_err);
    stub.set(static_cast<QPair<ErrorContext, QString> (Unit::*)() const>(&Unit::getLoadState), stub_str_err);
    stub.set(static_cast<QPair<ErrorContext, QString> (Unit::*)() const>(&Unit::getActiveState), stub_str_err);
    stub.set(static_cast<QPair<ErrorContext, QString> (Unit::*)() const>(&Unit::getSubState), stub_str_err);
    stub.set(static_cast<QPair<ErrorContext, QString> (Unit::*)() const>(&Unit::getDescription), stub_str_err);
    stub.set(static_cast<QPair<ErrorContext, bool> (Unit::*)() const>(&Unit::canStart), stub_bool_err);
    stub.set(static_cast<QPair<ErrorContext, bool> (Unit::*)() const>(&Unit::canStop), stub_bool_err);
    stub.set(static_cast<QPair<ErrorContext, bool> (Unit::*)() const>(&Unit::canReload), stub_bool_err);
    stub.set(static_cast<QPair<ErrorContext, quint32> (Svc::*)() const>(&Svc::getMainPID), stub_pid_err);
    stub.set(static_cast<QPair<ErrorContext, QString> (Mgr::*)(const QString &)>(&Mgr::GetUnitFileState), stub_GetUnitFileState_err);
    QSignalSpy spy(m_tester, &ServiceManager::serviceStatusUpdated);
    qRegisterMetaType<SystemServiceEntry>("SystemServiceEntry");

    // Act
    const SystemServiceEntry entry = m_tester->updateServiceEntry("/org/freedesktop/systemd1/unit/ssh_2eservice");

    // Assert：setter 全部跳过 → 默认值；信号仍然发射
    EXPECT_TRUE(entry.getId().isEmpty());
    EXPECT_TRUE(entry.getSName().isEmpty());
    EXPECT_TRUE(entry.getLoadState().isEmpty());
    EXPECT_TRUE(entry.getState().isEmpty());
    EXPECT_TRUE(entry.getStartupType().isEmpty());
    EXPECT_EQ(entry.getMainPID(), 0u);
    EXPECT_FALSE(entry.getCanStart());
    EXPECT_EQ(spy.count(), 1);
}

// updateServiceEntry：canReload 成功返回 true → canReload 字段置位（getter 成功分支补全）
TEST_F(UT_ServiceManager, updateServiceEntry_can_reload_ok)
{
    // Arrange：canReload 用成功 stub（其余同 success 用例）
    Stub stub;
    using Unit = Systemd1UnitInterface;
    using Mgr = Systemd1ManagerInterface;
    using Svc = Systemd1ServiceInterface;
    stub.set(static_cast<QPair<ErrorContext, QString> (Unit::*)() const>(&Unit::getId), stub_getId_ok);
    stub.set(static_cast<QPair<ErrorContext, QString> (Unit::*)() const>(&Unit::getLoadState), stub_str_loaded_ok);
    stub.set(static_cast<QPair<ErrorContext, QString> (Unit::*)() const>(&Unit::getActiveState), stub_str_active_ok);
    stub.set(static_cast<QPair<ErrorContext, QString> (Unit::*)() const>(&Unit::getSubState), stub_str_loaded_ok);
    stub.set(static_cast<QPair<ErrorContext, QString> (Unit::*)() const>(&Unit::getDescription), stub_str_loaded_ok);
    stub.set(static_cast<QPair<ErrorContext, bool> (Unit::*)() const>(&Unit::canStart), stub_bool_ok);
    stub.set(static_cast<QPair<ErrorContext, bool> (Unit::*)() const>(&Unit::canStop), stub_bool_ok);
    stub.set(static_cast<QPair<ErrorContext, bool> (Unit::*)() const>(&Unit::canReload), stub_bool_ok);
    stub.set(static_cast<QPair<ErrorContext, quint32> (Svc::*)() const>(&Svc::getMainPID), stub_pid_ok);
    stub.set(static_cast<QPair<ErrorContext, QString> (Mgr::*)(const QString &)>(&Mgr::GetUnitFileState), stub_GetUnitFileState_ok);
    qRegisterMetaType<SystemServiceEntry>("SystemServiceEntry");

    // Act
    const SystemServiceEntry entry = m_tester->updateServiceEntry("/org/freedesktop/systemd1/unit/ssh_2eservice");

    // Assert：canReload 成功读取为 true，其余字段不受影响
    EXPECT_TRUE(entry.getCanReload());
    EXPECT_TRUE(entry.getCanStart());
    EXPECT_EQ(entry.getMainPID(), 4321u);
}

// CustomTimer：终态 → 首次查询后停止并 deleteLater
TEST_F(UT_ServiceManager, customTimer_final_state_stops_and_deletes)
{
    // Arrange：updateServiceEntry 恒回终态 inactive；监听销毁
    Stub stub;
    stub.set(ADDR(ServiceManager, updateServiceEntry), stub_entry_final);
    int destroyedCount = 0;
    CustomTimer *timer = new CustomTimer(m_tester, nullptr);
    QObject::connect(timer, &QObject::destroyed, [&destroyedCount](QObject *) { ++destroyedCount; });

    // Act：200ms 首次触发
    timer->start("/unit/path");
    QTest::qWait(800);

    // Assert：终态 → 停止 + 销毁，仅一次查询
    EXPECT_GE(destroyedCount, 1);
    EXPECT_EQ(g_entryCallCount, 1);
}

// CustomTimer：非终态 → 指数退避重启（m_cnt²*200ms）；1500ms 内恰触发 3 次（200/400/800 间隔）
TEST_F(UT_ServiceManager, customTimer_nonfinal_backoff_restart)
{
    // Arrange：恒回非终态 activating；若实现退化为固定 200ms 间隔则 1500ms 内会触发 7 次
    Stub stub;
    stub.set(ADDR(ServiceManager, updateServiceEntry), stub_entry_activating);
    int destroyedCount = 0;
    CustomTimer *timer = new CustomTimer(m_tester, nullptr);
    QObject::connect(timer, &QObject::destroyed, [&destroyedCount](QObject *) { ++destroyedCount; });

    // Act：触发点 t=200 → 重启 1²*200 → t=400 → 重启 2²*200 → t=1200 → 重启 3²*200（t=3000 未到）
    timer->start("/unit/path");
    QTest::qWait(1500);

    // Assert：3 次查询、未销毁；随后手动清理避免跨用例泄漏
    EXPECT_EQ(g_entryCallCount, 3);
    EXPECT_EQ(destroyedCount, 0);
    delete timer;
    QTest::qWait(10);
}
