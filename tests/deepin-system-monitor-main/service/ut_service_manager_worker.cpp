// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// 用例计数声明（self-check-structural 验证此块）：
// | method | level | factors | min | actual |
// |--------|-------|---------|-----|--------|
// | startJob | high | complexity:19,cognitive:26 | 3 | 4 |
// | readUnitDescriptionFromUnitFile | low | - | 1 | 5 |
// | ServiceManagerWorker(ctor) | low | - | 1 | 1 |

//self
#include "service/service_manager_worker.h"

//dbus
#include "dbus/dbus_common.h"
#include "dbus/systemd1_manager_interface.h"
#include "dbus/systemd1_service_interface.h"
#include "dbus/systemd1_unit_interface.h"
#include "dbus/unit_file_info.h"
#include "dbus/unit_info.h"
//service
#include "service/service_manager.h"
#include "service/system_service_entry.h"

//qt
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>

//gtest
#include "stub.h"
#include <gtest/gtest.h>

/*
 * [test-types 最小清单完成情况]
 * 1. 复杂度>=10 方法分支覆盖：startJob（复杂度最高）+ readUnitDescriptionFromUnitFile 分支清单见下 — 完成
 * 2. AAA 三段注释：所有用例 Arrange/Act/Assert 分段 — 完成
 * 3. TEST_P 参数化 >=3 组同质输入：readUnitDescriptionFromUnitFile 4 组 — 完成
 * 4. EXPECT_THROW 精确异常：无抛异常 API，负面路径以 ErrorContext 错误分支 + 空结果覆盖 — 完成
 * 5. 外部依赖隔离：DBus 接口方法全部 stub（weak 符号补丁）；模板单元描述文件为测试自建 QTemporaryDir 临时文件 — 完成
 *
 * [startJob 分支清单]（复杂度>=10 强制枚举）
 *  B1  ListUnitFiles 失败 → 仅告警继续            — dbus_errors 用例 T 侧 / full 用例 F 侧
 *  B2  ListUnits 失败 → 仅告警继续                — dbus_errors 用例 T 侧 / full 用例 F 侧
 *  B3  units 哈希收集 endsWith(.service) T/F      — full 用例（ssh.service 入表 / cron.timer 排除）
 *  B4  fltUnits 过滤非 service 单元 T/F           — full 用例（ssh 保留 / cron.timer 过滤）
 *  B5  mapUnit 各 getter 失败/成功                — filestate_error 用例（GetUnitFileState 失败侧）/ full 用例（成功侧）
 *  B6  fltUnitFiles：basename 在哈希中 → 过滤 T   — full 用例（ssh.service 文件项被过滤）
 *  B7  fltUnitFiles：非 .service 文件 → 过滤 F    — full 用例（other.mount 被过滤）
 *  B8  mapUnitFiles：模板单元（sname 以 @ 结尾）→ 读文件描述 — full 用例（foo@）
 *  B9  mapUnitFiles：非模板单元 → DBus 取 Id/描述/状态/PID    — full 用例（bar.service）
 *  B10 resultReady 汇总发射                       — full/dbus_errors 用例
 * [readUnitDescriptionFromUnitFile 分支清单]
 *  fopen 失败 → 返回空；首行存在 → 预置 "Description=" 前缀取首行（疑似源码缺陷：未按行匹配 Description=，
 *  会对任意首行加前缀——用例按当前实现行为断言并在缺陷清单标红）；空文件 → 返回空
 */

namespace {

// ---------- stub 数据区 ----------
static ErrorContext g_listUnitFilesEc;
static UnitFileInfoList g_unitFiles;
static ErrorContext g_listUnitsEc;
static UnitInfoList g_units;
static QHash<QString, QString> g_idByPath;
static QHash<QString, QString> g_descByPath;
static QHash<QString, QString> g_activeByPath;
static QHash<QString, QString> g_loadByPath;
static QHash<QString, QString> g_subByPath;
static QHash<QString, quint32> g_pidByPath;
static bool g_canStart = true;
static bool g_canStop = true;
static bool g_canReload = false;
static bool g_getUnitFileStateFail = false;
static bool g_unitGettersFail = false;   // 全部 Unit/Svc getter 返回 DBus 错误
static QHash<QString, QString> g_fileStateByName;
static QString g_templPath;

static void resetStubData()
{
    g_listUnitFilesEc = ErrorContext();
    g_unitFiles.clear();
    g_listUnitsEc = ErrorContext();
    g_units.clear();
    g_idByPath.clear();
    g_descByPath.clear();
    g_activeByPath.clear();
    g_loadByPath.clear();
    g_subByPath.clear();
    g_pidByPath.clear();
    g_canStart = true;
    g_canStop = true;
    g_canReload = false;
    g_getUnitFileStateFail = false;
    g_unitGettersFail = false;
    g_fileStateByName.clear();
    g_templPath.clear();
}

// ---------- stub 函数区（替换签名 = 成员函数去掉 this；带参成员以 self 占位首参对齐 ABI） ----------
static QPair<ErrorContext, UnitFileInfoList> stub_ListUnitFiles(Systemd1ManagerInterface *)
{
    return { g_listUnitFilesEc, g_unitFiles };
}

static QPair<ErrorContext, UnitInfoList> stub_ListUnits(Systemd1ManagerInterface *)
{
    return { g_listUnitsEc, g_units };
}

static QPair<ErrorContext, QString> stub_GetUnitFileState(Systemd1ManagerInterface *, const QString &unit)
{
    if (g_getUnitFileStateFail) {
        return { ErrorContext(ErrorContext::kErrorTypeDBus, 3, "org.freedesktop.systemd1.NoSuchUnit", "no state"), QString() };
    }
    return { ErrorContext(), g_fileStateByName.value(unit) };
}

static QPair<ErrorContext, QString> stub_getId(Systemd1UnitInterface *self)
{
    if (g_unitGettersFail)
        return { ErrorContext(ErrorContext::kErrorTypeDBus, 3, "org.freedesktop.systemd1.NoSuchUnit", "no unit"), QString() };
    return { ErrorContext(), g_idByPath.value(self->path()) };
}

static QPair<ErrorContext, QString> stub_getDescription(Systemd1UnitInterface *self)
{
    if (g_unitGettersFail)
        return { ErrorContext(ErrorContext::kErrorTypeDBus, 3, "org.freedesktop.systemd1.NoSuchUnit", "no unit"), QString() };
    return { ErrorContext(), g_descByPath.value(self->path()) };
}

static QPair<ErrorContext, QString> stub_getActiveState(Systemd1UnitInterface *self)
{
    if (g_unitGettersFail)
        return { ErrorContext(ErrorContext::kErrorTypeDBus, 3, "org.freedesktop.systemd1.NoSuchUnit", "no unit"), QString() };
    return { ErrorContext(), g_activeByPath.value(self->path()) };
}

static QPair<ErrorContext, QString> stub_getLoadState(Systemd1UnitInterface *self)
{
    if (g_unitGettersFail)
        return { ErrorContext(ErrorContext::kErrorTypeDBus, 3, "org.freedesktop.systemd1.NoSuchUnit", "no unit"), QString() };
    return { ErrorContext(), g_loadByPath.value(self->path()) };
}

static QPair<ErrorContext, QString> stub_getSubState(Systemd1UnitInterface *self)
{
    if (g_unitGettersFail)
        return { ErrorContext(ErrorContext::kErrorTypeDBus, 3, "org.freedesktop.systemd1.NoSuchUnit", "no unit"), QString() };
    return { ErrorContext(), g_subByPath.value(self->path()) };
}

static QPair<ErrorContext, bool> stub_canStart(Systemd1UnitInterface *)
{
    if (g_unitGettersFail)
        return { ErrorContext(ErrorContext::kErrorTypeDBus, 3, "org.freedesktop.systemd1.NoSuchUnit", "no unit"), false };
    return { ErrorContext(), g_canStart };
}

static QPair<ErrorContext, bool> stub_canStop(Systemd1UnitInterface *)
{
    if (g_unitGettersFail)
        return { ErrorContext(ErrorContext::kErrorTypeDBus, 3, "org.freedesktop.systemd1.NoSuchUnit", "no unit"), false };
    return { ErrorContext(), g_canStop };
}

static QPair<ErrorContext, bool> stub_canReload(Systemd1UnitInterface *)
{
    if (g_unitGettersFail)
        return { ErrorContext(ErrorContext::kErrorTypeDBus, 3, "org.freedesktop.systemd1.NoSuchUnit", "no unit"), false };
    return { ErrorContext(), g_canReload };
}

static QPair<ErrorContext, quint32> stub_getMainPID(Systemd1ServiceInterface *self)
{
    if (g_unitGettersFail)
        return { ErrorContext(ErrorContext::kErrorTypeDBus, 3, "org.freedesktop.systemd1.NoSuchUnit", "no unit"), quint32(0) };
    return { ErrorContext(), g_pidByPath.value(self->path()) };
}

static void installDBusStubs(Stub &stub)
{
    using Mgr = Systemd1ManagerInterface;
    using Unit = Systemd1UnitInterface;
    using Svc = Systemd1ServiceInterface;
    stub.set(static_cast<QPair<ErrorContext, UnitFileInfoList> (Mgr::*)()>(&Mgr::ListUnitFiles), stub_ListUnitFiles);
    stub.set(static_cast<QPair<ErrorContext, UnitInfoList> (Mgr::*)()>(&Mgr::ListUnits), stub_ListUnits);
    stub.set(static_cast<QPair<ErrorContext, QString> (Mgr::*)(const QString &)>(&Mgr::GetUnitFileState), stub_GetUnitFileState);
    stub.set(static_cast<QPair<ErrorContext, QString> (Unit::*)() const>(&Unit::getId), stub_getId);
    stub.set(static_cast<QPair<ErrorContext, QString> (Unit::*)() const>(&Unit::getDescription), stub_getDescription);
    stub.set(static_cast<QPair<ErrorContext, QString> (Unit::*)() const>(&Unit::getActiveState), stub_getActiveState);
    stub.set(static_cast<QPair<ErrorContext, QString> (Unit::*)() const>(&Unit::getLoadState), stub_getLoadState);
    stub.set(static_cast<QPair<ErrorContext, QString> (Unit::*)() const>(&Unit::getSubState), stub_getSubState);
    stub.set(static_cast<QPair<ErrorContext, bool> (Unit::*)() const>(&Unit::canStart), stub_canStart);
    stub.set(static_cast<QPair<ErrorContext, bool> (Unit::*)() const>(&Unit::canStop), stub_canStop);
    stub.set(static_cast<QPair<ErrorContext, bool> (Unit::*)() const>(&Unit::canReload), stub_canReload);
    stub.set(static_cast<QPair<ErrorContext, quint32> (Svc::*)() const>(&Svc::getMainPID), stub_getMainPID);
}

}  // namespace

class UT_ServiceManagerWorker : public ::testing::Test
{
public:
    UT_ServiceManagerWorker() : m_tester(nullptr) {}

    static void SetUpTestSuite()
    {
        // resultReady(QList<SystemServiceEntry>) 供 QSignalSpy 捕获所需元类型
        qRegisterMetaType<SystemServiceEntry>("SystemServiceEntry");
        qRegisterMetaType<QList<SystemServiceEntry>>("QList<SystemServiceEntry>");
    }

    void SetUp() override
    {
        // Arrange：被测对象 + stub 数据复位
        resetStubData();
        m_tester = new ServiceManagerWorker();
    }

    void TearDown() override
    {
        delete m_tester;
        m_tester = nullptr;
        resetStubData();
    }

protected:
    ServiceManagerWorker *m_tester;
};

// 构造：无外部依赖，无副作用
TEST_F(UT_ServiceManagerWorker, ctor_no_side_effect)
{
    // Act：m_tester 已在 SetUp 构造
    // Assert：对象可用（非空指针）
    EXPECT_NE(m_tester, nullptr);
}

// startJob 全流水线：units 过滤+映射、unitFiles 哈希过滤+模板/非模板映射、resultReady 汇总
TEST_F(UT_ServiceManagerWorker, startJob_full_pipeline)
{
    // Arrange：自建模板单元文件（隔离真实文件系统）
    QTemporaryDir tdir;
    ASSERT_TRUE(tdir.isValid());
    const QString tplPath = tdir.path() + "/foo@.service";
    {
        QFile f(tplPath);
        ASSERT_TRUE(f.open(QIODevice::WriteOnly | QIODevice::Text));
        f.write("Description=Foo Template\n");
        f.close();
    }
    g_templPath = tplPath;

    const QString sshPath = "/org/freedesktop/systemd1/unit/ssh_2eservice";
    const QString barPath = Systemd1UnitInterface::normalizeUnitPath("bar.service").path();

    UnitInfo sshUnit;
    sshUnit.setName("ssh.service");
    sshUnit.setLoadState("loaded");
    sshUnit.setActiveState("active");
    sshUnit.setSubState("running");
    sshUnit.setUnitObjectPath(sshPath);
    sshUnit.setDescription("OpenBSD Secure Shell server");
    UnitInfo timerUnit;
    timerUnit.setName("cron.timer");
    g_units << sshUnit << timerUnit;

    g_unitFiles << UnitFileInfo("/usr/lib/systemd/system/ssh.service", "enabled")   // B6: 哈希命中过滤
                << UnitFileInfo(tplPath, "static")                                  // B8: 模板
                << UnitFileInfo("/usr/lib/systemd/system/other.mount", "enabled")   // B7: 非 service 过滤
                << UnitFileInfo("/usr/lib/systemd/system/bar.service", "disabled"); // B9: 非模板

    g_idByPath.insert(sshPath, "ssh.service");
    g_idByPath.insert(barPath, "bar.service");
    g_descByPath.insert(sshPath, "OpenBSD Secure Shell server");
    g_descByPath.insert(barPath, "Bar service");
    g_activeByPath.insert(barPath, "inactive");
    g_loadByPath.insert(barPath, "loaded");
    g_subByPath.insert(barPath, "dead");
    g_pidByPath.insert(sshPath, 1234u);
    g_pidByPath.insert(barPath, 99u);
    g_fileStateByName.insert("ssh.service", "enabled");

    Stub stub;
    installDBusStubs(stub);
    QSignalSpy spy(m_tester, &ServiceManagerWorker::resultReady);

    // Act
    m_tester->startJob();

    // Assert：1 次信号，3 条目（ssh 来自单元表；foo@、bar 来自文件表）
    ASSERT_EQ(spy.count(), 1);
    const QList<SystemServiceEntry> list = spy.takeFirst().at(0).value<QList<SystemServiceEntry>>();
    ASSERT_EQ(list.size(), 3);

    const SystemServiceEntry &ssh = list.at(0);
    EXPECT_EQ(ssh.getId(), "ssh.service");
    EXPECT_EQ(ssh.getSName(), "ssh");
    EXPECT_EQ(ssh.getLoadState(), "loaded");
    EXPECT_EQ(ssh.getActiveState(), "active");
    EXPECT_EQ(ssh.getSubState(), "running");
    EXPECT_EQ(ssh.getUnitObjectPath(), sshPath);
    EXPECT_EQ(ssh.getDescription(), "OpenBSD Secure Shell server");
    EXPECT_EQ(ssh.getMainPID(), 1234u);
    EXPECT_TRUE(ssh.getCanStart());
    EXPECT_TRUE(ssh.getCanStop());
    EXPECT_FALSE(ssh.getCanReload());
    EXPECT_EQ(ssh.getState(), "enabled");
    EXPECT_EQ(ssh.getStartupType(), QString(kServiceAutoStartup));

    const SystemServiceEntry &foo = list.at(1);
    // 模板分支：不取 Id/DBus 状态，仅读文件描述
    EXPECT_TRUE(foo.getId().isEmpty());
    EXPECT_EQ(foo.getSName(), "foo@");
    EXPECT_EQ(foo.getState(), "static");
    EXPECT_EQ(foo.getStartupType(), QString(kServiceNAStartup));
    // 当前实现行为：对文件首行整体预置 "Description=" 前缀（疑似源码缺陷，已标红报告）
    EXPECT_EQ(foo.getDescription(), QString("Description=Description=Foo Template"));

    const SystemServiceEntry &bar = list.at(2);
    EXPECT_EQ(bar.getId(), "bar.service");
    EXPECT_EQ(bar.getSName(), "bar");
    EXPECT_EQ(bar.getState(), "disabled");
    EXPECT_EQ(bar.getStartupType(), QString(kServiceManualStartup));
    EXPECT_EQ(bar.getDescription(), "Bar service");
    EXPECT_EQ(bar.getActiveState(), "inactive");
    EXPECT_EQ(bar.getLoadState(), "loaded");
    EXPECT_EQ(bar.getSubState(), "dead");
    EXPECT_EQ(bar.getMainPID(), 99u);
    // 文件表分支不设置能力位（与单元表分支差异）
    EXPECT_FALSE(bar.getCanStart());
}

// startJob 错误韧性：ListUnitFiles/ListUnits 全部失败 → 仍发射 resultReady，结果为空表
TEST_F(UT_ServiceManagerWorker, startJob_dbus_errors_emits_empty_result)
{
    // Arrange
    g_listUnitFilesEc = ErrorContext(ErrorContext::kErrorTypeDBus, 2, "org.freedesktop.DBus.Error.AccessDenied", "denied");
    g_listUnitsEc = ErrorContext(ErrorContext::kErrorTypeDBus, 2, "org.freedesktop.DBus.Error.AccessDenied", "denied");
    Stub stub;
    installDBusStubs(stub);
    QSignalSpy spy(m_tester, &ServiceManagerWorker::resultReady);

    // Act
    m_tester->startJob();

    // Assert：不崩溃、仍发射一次、空结果
    ASSERT_EQ(spy.count(), 1);
    const QList<SystemServiceEntry> list = spy.takeFirst().at(0).value<QList<SystemServiceEntry>>();
    EXPECT_TRUE(list.isEmpty());
}

// startJob 部分失败：GetUnitFileState 失败 → id 已设置、state 为空、startupType 降级为 N/A
TEST_F(UT_ServiceManagerWorker, startJob_unitFileState_error_falls_back_to_na)
{
    // Arrange
    const QString sshPath = "/org/freedesktop/systemd1/unit/ssh_2eservice";
    UnitInfo sshUnit;
    sshUnit.setName("ssh.service");
    sshUnit.setLoadState("loaded");
    sshUnit.setActiveState("inactive");
    sshUnit.setSubState("dead");
    sshUnit.setUnitObjectPath(sshPath);
    g_units << sshUnit;
    g_idByPath.insert(sshPath, "ssh.service");
    g_pidByPath.insert(sshPath, 7u);
    g_getUnitFileStateFail = true;

    Stub stub;
    installDBusStubs(stub);
    QSignalSpy spy(m_tester, &ServiceManagerWorker::resultReady);

    // Act
    m_tester->startJob();

    // Assert
    ASSERT_EQ(spy.count(), 1);
    const QList<SystemServiceEntry> list = spy.takeFirst().at(0).value<QList<SystemServiceEntry>>();
    ASSERT_EQ(list.size(), 1);
    EXPECT_EQ(list.at(0).getId(), "ssh.service");
    EXPECT_TRUE(list.at(0).getState().isEmpty());
    EXPECT_EQ(list.at(0).getStartupType(), QString(kServiceNAStartup));
    EXPECT_EQ(list.at(0).getMainPID(), 7u);
}

// startJob：单元/单元文件全部 getter 返回 DBus 错误 → 各 qCWarning 分支 + 条目保持默认
TEST_F(UT_ServiceManagerWorker, startJob_all_getters_fail_default_entry)
{
    // Arrange：一个运行单元 + 一个仅存在于单元文件列表的条目；所有 Unit/Svc getter 失败
    const QString sshPath = "/org/freedesktop/systemd1/unit/ssh_2eservice";
    UnitInfo sshUnit;
    sshUnit.setName("ssh.service");
    sshUnit.setLoadState("loaded");
    sshUnit.setActiveState("active");
    sshUnit.setSubState("running");
    sshUnit.setUnitObjectPath(sshPath);
    g_units << sshUnit;
    g_idByPath.insert(sshPath, "ssh.service");
    g_pidByPath.insert(sshPath, 7u);

    UnitFileInfo barFile;
    barFile.setName("bar.service");
    g_unitFiles << barFile;   // 文件侧条目：mapUnitFiles 的 getter 失败分支
    g_unitGettersFail = true;

    Stub stub;
    installDBusStubs(stub);
    QSignalSpy spy(m_tester, &ServiceManagerWorker::resultReady);

    // Act
    m_tester->startJob();

    // Assert：列表仍发出；ssh 条目字段保持默认（id/pid/state 空，canX false）；
    // bar.service 文件条目 id 也为空 → 不在 hash 中 → 仍然入表
    ASSERT_EQ(spy.count(), 1);
    const QList<SystemServiceEntry> list = spy.takeFirst().at(0).value<QList<SystemServiceEntry>>();
    ASSERT_EQ(list.size(), 2);
    EXPECT_EQ(list.at(0).getSName(), "ssh");
    EXPECT_EQ(list.at(0).getLoadState(), "loaded");   // UnitInfo 直读字段不受 getter 失败影响
    EXPECT_TRUE(list.at(0).getId().isEmpty());
    EXPECT_TRUE(list.at(0).getState().isEmpty());
    EXPECT_FALSE(list.at(0).getCanStart());
    EXPECT_FALSE(list.at(0).getCanStop());
    EXPECT_FALSE(list.at(0).getCanReload());
    EXPECT_EQ(list.at(0).getMainPID(), 0u);
    EXPECT_EQ(list.at(0).getStartupType(), QString(kServiceNAStartup));
    EXPECT_TRUE(list.at(1).getId().isEmpty());   // bar.service：DBus 失败 → id 不写入（仅 sname/state 来自文件名）
    EXPECT_EQ(list.at(1).getSName(), "bar");
}

// readUnitDescriptionFromUnitFile TEST_P：4 组同质输入（文件内容 → 期望输出）
namespace {
struct ReadDescCase
{
    QString fileContent;   // 空串表示不创建文件
    QString expected;
};
}  // namespace

class UT_ServiceManagerWorker_ReadDesc : public ::testing::TestWithParam<ReadDescCase>
{
};

// 访问私有静态方法：测试编译开启 -fno-access-control
TEST_P(UT_ServiceManagerWorker_ReadDesc, reads_first_line_with_prefix)
{
    // Arrange
    const ReadDescCase &cs = GetParam();
    QString path;
    if (!cs.fileContent.isEmpty()) {
        QTemporaryDir tdir;
        ASSERT_TRUE(tdir.isValid());
        path = tdir.path() + "/tpl@.service";
        QFile f(path);
        ASSERT_TRUE(f.open(QIODevice::WriteOnly | QIODevice::Text));
        f.write(cs.fileContent.toUtf8());
        f.close();

        // Act
        const QString desc = ServiceManagerWorker::readUnitDescriptionFromUnitFile(path);

        // Assert（按当前实现行为：首行前预置 "Description="）
        EXPECT_EQ(desc, cs.expected);
    } else {
        // Act：不存在的路径 → fopen 失败分支
        const QString desc = ServiceManagerWorker::readUnitDescriptionFromUnitFile("/nonexistent/sm-ut/no_such_@.service");

        // Assert
        EXPECT_TRUE(desc.isEmpty());
    }
}

INSTANTIATE_TEST_SUITE_P(FileInputs, UT_ServiceManagerWorker_ReadDesc,
                         ::testing::Values(ReadDescCase{"Description=Foo Template\n", "Description=Description=Foo Template"},
                                           ReadDescCase{"[Unit]\nDescription=Foo\n", "Description=[Unit]"},
                                           ReadDescCase{"\n", "Description="},
                                           ReadDescCase{QString(), QString()}));

// readUnitDescriptionFromUnitFile：文件存在但为 0 字节 → fgets 立即 EOF → 警告并返回空
TEST_F(UT_ServiceManagerWorker, readDesc_empty_file_returns_empty_with_warning)
{
    // Arrange：创建 0 字节模板单元文件
    QTemporaryDir tdir;
    ASSERT_TRUE(tdir.isValid());
    const QString path = tdir.path() + "/empty@.service";
    QFile f(path);
    ASSERT_TRUE(f.open(QIODevice::WriteOnly));
    f.close();

    // Act
    const QString desc = ServiceManagerWorker::readUnitDescriptionFromUnitFile(path);

    // Assert：循环未读到任何行 → b=false → "No description found" 分支（L315-316）
    EXPECT_TRUE(desc.isEmpty());
}
