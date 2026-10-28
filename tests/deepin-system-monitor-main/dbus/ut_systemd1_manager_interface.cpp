// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// UT_Systemd1ManagerInterface：systemd1 Manager DBus 接口封装单元测试（qt-autotest-generator 重写）
//
// 被测文件：deepin-system-monitor-main/dbus/systemd1_manager_interface.cpp
//          （9 个 DBus 方法为头文件 inline 实现，覆盖归属 systemd1_manager_interface.h）
// 重写原因：Mode 6 审查判 FAIL（原用例断言空洞 EMPTY_ASSERT，未验证 ErrorContext/返回值语义）
//
// 打桩说明（外部依赖全部 stub，不依赖真实 systemd 总线）：
// - QDBusConnection("ut-fake")：未连接连接，未打桩时 callWithArgumentList
//   确定性地返回 Disconnected 错误（无任何真实总线 I/O）
// - QDBusMessage::type/signature/arguments：受控回复内容（成功/签名不符/载荷解析）
//
// ============================ 用例计数声明 ============================
// 方法                              | level | 最少用例 | actual | 说明
// Systemd1ManagerInterface()        | mid   | 1 | 1 | 构造（含 staticInterfaceName 校验）
// ~Systemd1ManagerInterface()       | low   | 1 | - | 由 TearDown delete 覆盖
// ListUnitFiles()                   | mid   | 2 | 3 | 断连错误 / 签名不符 / a(ss) 载荷解析
// ListUnits()                       | mid   | 2 | 3 | 断连错误 / 签名不符 / a(ssssssouso) 载荷解析
// GetUnit(const QString&)           | mid   | 2 | 3 | TEST_P：断连/签名不符/成功解析 o
// StartUnit(const QString&,mode)    | mid   | 2 | 3 | 同上（TEST_P 四方法共享）
// StopUnit(const QString&,mode)     | mid   | 2 | 3 | 同上
// RestartUnit(const QString&,mode)  | mid   | 2 | 3 | 同上
// GetUnitFileState(const QString&)  | mid   | 2 | 3 | 断连 / 签名不符 / 成功解析 s
// EnableUnitFiles(list,rt,force)    | mid   | 2 | 2 | 断连错误 / 成功回复保持默认结果（TODO 未解析）
// DisableUnitFiles(list,rt)         | mid   | 2 | 2 | 断连错误 / 成功回复保持默认结果（TODO 未解析）
//
// ============================== 最小清单 ==============================
// [x] 1 公开方法已列出，每个方法 ≥ 1 用例
// [x] 2 输入按等价类划分：错误回复 / 回复签名不符 / 回复签名匹配
// [x] 3 边界值显式覆盖：签名匹配为解析前提（错误→不解析→空结果）
// [x] 4 同质多组输入用 TEST_P 参数化（4 个对象路径方法 × 3 分支）
// [x] 5 分支清单已列出并映射到用例名
// [x] 6 每条 if/early-return 分支有触发用例（ErrorMessage / 签名不符 / 签名匹配）
// [x] 7 本类无显式 throw，不适用 EXPECT_THROW
// [x] 8 负面场景（断连错误、签名不符→空结果）有专门用例
// [x] 9 负面用例验证强异常安全（错误后返回空结果与错误上下文，不抛异常）
// [x] 10 项目内接口类用 gMock，其他用 stub_ext（本项目统一用项目自带 stub.h 打桩）
//
// ============================== 分支清单 ==============================
// （来源：工作目录源码 systemd1_manager_interface.h）
//   B1  reply.type()==ErrorMessage → ec=kErrorTypeDBus+错误名/消息 → 各方法 *_OnDisconnectedBus_*
//   B2  reply 为成功但签名不符 → 跳过解析，结果保持空      → 各方法 *_OnWrongReplySignature_*
//   B3  reply 为成功且签名匹配 → 解析载荷                  → 各方法 *_OnValidReply_*
//   B4  EnableUnitFiles/DisableUnitFiles 成功分支为 TODO 空实现 → 保持默认结果（记录为未完成实现）
//
// 疑似源码缺陷记录（不改源码，只标红）：
//   #D-Systemd1Manager-01 EnableUnitFiles/DisableUnitFiles 成功回复未解析（TODO "half baked"），
//      调用方拿到的 result 恒为默认值（systemd1_manager_interface.h:300-341）

#include "dbus/systemd1_manager_interface.h"
//self
#include "common/error_context.h"
#include "dbus/unit_file_info.h"
#include "dbus/unit_info.h"
//qt
#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusError>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QList>
#include <QString>
#include <QVariant>
//gtest
#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
//stub
#include "stub.h"

using namespace testing;

// ============================== 打桩实现 ==============================
namespace {
QDBusMessage::MessageType g_msgType = QDBusMessage::ReplyMessage;
QString g_msgSignature;
QList<QVariant> g_msgArguments;

void resetSystemdStubState()
{
    g_msgType = QDBusMessage::ReplyMessage;
    g_msgSignature.clear();
    g_msgArguments.clear();
}

// QDBusMessage::MessageType QDBusMessage::type()
static QDBusMessage::MessageType stub_QDBusMessage_type()
{
    return g_msgType;
}

// QString QDBusMessage::signature()
static QString stub_QDBusMessage_signature()
{
    return g_msgSignature;
}

// const QList<QVariant> &QDBusMessage::arguments()
static QList<QVariant> stub_QDBusMessage_arguments()
{
    return g_msgArguments;
}

// 公共打桩：受控回复三件套
static void applyReplyStub(Stub &stub)
{
    stub.set(ADDR(QDBusMessage, type), stub_QDBusMessage_type);
    stub.set(ADDR(QDBusMessage, signature), stub_QDBusMessage_signature);
    stub.set((const QList<QVariant> &(QDBusMessage::*)())ADDR(QDBusMessage, arguments),
             stub_QDBusMessage_arguments);
}

// 载荷：a(ss) 的 UnitFileInfoList
static QList<QVariant> makeUnitFilesArgs()
{
    QDBusArgument arg;
    arg.beginArray(qMetaTypeId<UnitFileInfo>());
    arg.beginStructure();
    arg << QStringLiteral("ut-a.service") << QStringLiteral("enabled");
    arg.endStructure();
    arg.beginStructure();
    arg << QStringLiteral("ut-b.timer") << QStringLiteral("disabled");
    arg.endStructure();
    arg.endArray();
    return QList<QVariant>{QVariant::fromValue<QDBusArgument>(arg)};
}

// 载荷：a(ssssssouso) 的 UnitInfoList
static QList<QVariant> makeUnitsArgs()
{
    QDBusArgument arg;
    arg.beginArray(qMetaTypeId<UnitInfo>());
    arg.beginStructure();
    arg << QStringLiteral("ut.service") << QStringLiteral("UT Unit") << QStringLiteral("loaded")
        << QStringLiteral("active") << QStringLiteral("running") << QStringLiteral("-")
        << QDBusObjectPath(QStringLiteral("/org/freedesktop/systemd1/unit/ut_2eservice"))
        << quint32(7) << QStringLiteral("start")
        << QDBusObjectPath(QStringLiteral("/org/freedesktop/systemd1/job/777"));
    arg.endStructure();
    arg.endArray();
    return QList<QVariant>{QVariant::fromValue<QDBusArgument>(arg)};
}

// 载荷：单个对象路径（GetUnit/StartUnit/StopUnit/RestartUnit）
static QList<QVariant> makeObjectPathArgs()
{
    return QList<QVariant>{
        QVariant::fromValue(QDBusObjectPath(QStringLiteral("/org/freedesktop/systemd1/job/777")))};
}

// ---- QDBusArgument 读原语打桩 ----
// 合成载荷是 write-only 的 QDBusArgument，无法真实 demarshall（Qt 限制）。
// 参照 daemon_ut 既有模式：stub 读原语回灌夹具数据，被测 operator>> 函数体真实执行。
// 注意：stub 补丁进程内全局生效，仅应与覆盖 ListUnitFiles/ListUnits 的过滤器同用。
static QStringList g_fedStrings;
static QList<uint> g_fedUInts;
static QList<bool> g_fedAtEnd;

static void stub_QDBusArgument_beginStructureR(const void *) { }
static void stub_QDBusArgument_endStructureR(const void *) { }
static void stub_QDBusArgument_beginArrayR(const void *) { }
static void stub_QDBusArgument_endArrayR(const void *) { }
static bool stub_QDBusArgument_atEnd(const void *)
{
    return g_fedAtEnd.isEmpty() ? true : g_fedAtEnd.takeFirst();
}
static const QDBusArgument &stub_QDBusArgument_readString(const void *self, QString &out)
{
    out = g_fedStrings.isEmpty() ? QString() : g_fedStrings.takeFirst();
    return *static_cast<const QDBusArgument *>(self);
}
static const QDBusArgument &stub_QDBusArgument_readUInt(const void *self, uint &out)
{
    out = g_fedUInts.isEmpty() ? 0u : g_fedUInts.takeFirst();
    return *static_cast<const QDBusArgument *>(self);
}

// 为载荷解析测试安装读原语打桩（beginArray/endArray/结构体/atEnd/QString/uint）
static void applyArgumentReadPrimitives(Stub &stub)
{
    stub.set((void (QDBusArgument::*)() const)ADDR(QDBusArgument, beginStructure),
             stub_QDBusArgument_beginStructureR);
    stub.set((void (QDBusArgument::*)() const)ADDR(QDBusArgument, endStructure),
             stub_QDBusArgument_endStructureR);
    stub.set((void (QDBusArgument::*)() const)ADDR(QDBusArgument, beginArray),
             stub_QDBusArgument_beginArrayR);
    stub.set((void (QDBusArgument::*)() const)ADDR(QDBusArgument, endArray),
             stub_QDBusArgument_endArrayR);
    stub.set((bool (QDBusArgument::*)() const)ADDR(QDBusArgument, atEnd), stub_QDBusArgument_atEnd);
    stub.set((const QDBusArgument &(QDBusArgument::*)(QString &) const)ADDR(QDBusArgument, operator>>),
             stub_QDBusArgument_readString);
    stub.set((const QDBusArgument &(QDBusArgument::*)(uint &) const)ADDR(QDBusArgument, operator>>),
             stub_QDBusArgument_readUInt);
    g_fedStrings.clear();
    g_fedUInts.clear();
    g_fedAtEnd.clear();
}

// 载荷：单个字符串（GetUnitFileState）
static QList<QVariant> makeStringArgs()
{
    return QList<QVariant>{QVariant(QStringLiteral("enabled"))};
}
}  // namespace

// ============================== 夹具 ==============================
class UT_Systemd1ManagerInterface : public ::testing::Test
{
public:
    UT_Systemd1ManagerInterface() : m_conn(QStringLiteral("ut-systemd-fake")) {}
    ~UT_Systemd1ManagerInterface() override {}

    void SetUp() override
    {
        resetSystemdStubState();
        m_conn = QDBusConnection(QStringLiteral("ut-systemd-fake"));
        m_tester = new Systemd1ManagerInterface(QStringLiteral("org.freedesktop.systemd1"),
                                                QStringLiteral("/org/freedesktop/systemd1"),
                                                m_conn, nullptr);
    }

    void TearDown() override
    {
        delete m_tester;
        m_tester = nullptr;
    }

    QDBusConnection m_conn;
    Systemd1ManagerInterface *m_tester = nullptr;
};

// 对象路径返回类方法的参数化夹具（GetUnit/StartUnit/StopUnit/RestartUnit 共享分支语义）
namespace {
enum PathMethod { M_GetUnit = 0, M_StartUnit, M_StopUnit, M_RestartUnit };
struct PathMethodParam {
    PathMethod method;
    QString arg1;   // path / name
    QString arg2;   // mode（GetUnit 不使用）
};
const char *pathMethodLabel(PathMethod m)
{
    switch (m) {
    case M_GetUnit: return "GetUnit";
    case M_StartUnit: return "StartUnit";
    case M_StopUnit: return "StopUnit";
    case M_RestartUnit: return "RestartUnit";
    }
    return "Unknown";
}
}  // namespace

class UT_Systemd1Manager_PathMethod : public UT_Systemd1ManagerInterface,
                                      public ::testing::WithParamInterface<PathMethodParam>
{
public:
    // 分派调用对应方法
    QPair<ErrorContext, QDBusObjectPath> invoke(const PathMethodParam &p)
    {
        switch (p.method) {
        case M_StartUnit: return m_tester->StartUnit(p.arg1, p.arg2);
        case M_StopUnit: return m_tester->StopUnit(p.arg1, p.arg2);
        case M_RestartUnit: return m_tester->RestartUnit(p.arg1, p.arg2);
        case M_GetUnit:
        default: return m_tester->GetUnit(p.arg1);
        }
    }
};

INSTANTIATE_TEST_SUITE_P(PathMethods, UT_Systemd1Manager_PathMethod, ::testing::Values(
                             PathMethodParam { M_GetUnit, "/org/freedesktop/systemd1/unit/ut", "" },
                             PathMethodParam { M_StartUnit, "ut.service", "replace" },
                             PathMethodParam { M_StopUnit, "ut.service", "replace" },
                             PathMethodParam { M_RestartUnit, "ut.service", "fail" }
                             ));

// ============================== 用例 ==============================
// 构造与接口名
TEST_F(UT_Systemd1ManagerInterface, CtorDtor_ConstructWithFakeConnection_NoCrash)
{
    // Arrange: SetUp 已构造对象
    // Act: SetUp 已构造对象（TearDown 负责 delete）
    // Assert: 接口名正确且对象有效
    EXPECT_STREQ(Systemd1ManagerInterface::staticInterfaceName(), "org.freedesktop.systemd1.Manager");
    EXPECT_NE(m_tester, nullptr);
}

// ---- ListUnitFiles ----
// B1: 断连总线 → DBus 错误上下文 + 空结果
TEST_F(UT_Systemd1ManagerInterface, ListUnitFiles_OnDisconnectedBus_SetsDBusErrorContext)
{
    // Arrange: 未连接连接（无打桩），调用必然失败
    // Act: 调用
    const auto result = m_tester->ListUnitFiles();
    // Assert: 错误上下文为 DBus 且结果为空
    EXPECT_EQ(result.first.getCode(), ErrorContext::kErrorTypeDBus);
    EXPECT_TRUE(result.second.isEmpty());
    EXPECT_TRUE(result.second.empty());
    EXPECT_EQ(result.first.getErrorName(), QStringLiteral("org.freedesktop.DBus.Error.Disconnected"));
}

// B2: 成功回复但签名不符 → 跳过解析，结果为空且无错误
TEST_F(UT_Systemd1ManagerInterface, ListUnitFiles_OnWrongReplySignature_KeepsResultEmpty)
{
    // Arrange: 受控成功回复 + 错误签名
    Stub stub;
    applyReplyStub(stub);
    g_msgType = QDBusMessage::ReplyMessage;
    g_msgSignature = QStringLiteral("xx");

    // Act: 调用
    const auto result = m_tester->ListUnitFiles();

    // Assert: 无错误且空结果
    EXPECT_EQ(result.first.getCode(), ErrorContext::kErrorTypeNoError);
    EXPECT_TRUE(result.second.empty());
}

// B3: 成功回复 a(ss) → 解析出单元文件列表
TEST_F(UT_Systemd1ManagerInterface, ListUnitFiles_OnValidReply_ParsesUnitFileList)
{
    // Arrange: 受控成功回复 + a(ss) 载荷；读原语回灌两条记录
    Stub stub;
    applyReplyStub(stub);
    applyArgumentReadPrimitives(stub);
    g_msgType = QDBusMessage::ReplyMessage;
    g_msgSignature = QStringLiteral("a(ss)");
    g_msgArguments = makeUnitFilesArgs();
    g_fedAtEnd << false << false;   // 两轮循环
    g_fedStrings << QStringLiteral("ut-a.service") << QStringLiteral("enabled")
                 << QStringLiteral("ut-b.timer") << QStringLiteral("disabled");

    // Act: 调用
    const auto result = m_tester->ListUnitFiles();

    // Assert: 无错误且两条记录值保持
    EXPECT_EQ(result.first.getCode(), ErrorContext::kErrorTypeNoError);
    EXPECT_EQ(result.second.size(), 2);
    if (result.second.size() == 2) {
        EXPECT_EQ(result.second.at(0).getName(), QStringLiteral("ut-a.service"));
        EXPECT_EQ(result.second.at(0).getStatus(), QStringLiteral("enabled"));
        EXPECT_EQ(result.second.at(1).getName(), QStringLiteral("ut-b.timer"));
        EXPECT_EQ(result.second.at(1).getStatus(), QStringLiteral("disabled"));
    }
}

// ---- ListUnits ----
// B1: 断连总线 → DBus 错误上下文 + 空结果
TEST_F(UT_Systemd1ManagerInterface, ListUnits_OnDisconnectedBus_SetsDBusErrorContext)
{
    // Arrange: 未连接连接（无打桩）
    // Act: 调用
    const auto result = m_tester->ListUnits();
    // Assert: 错误上下文为 DBus 且结果为空
    EXPECT_EQ(result.first.getCode(), ErrorContext::kErrorTypeDBus);
    EXPECT_TRUE(result.second.empty());
    EXPECT_EQ(result.first.getErrorName(), QStringLiteral("org.freedesktop.DBus.Error.Disconnected"));
}

// B2: 成功回复但签名不符 → 跳过解析
TEST_F(UT_Systemd1ManagerInterface, ListUnits_OnWrongReplySignature_KeepsResultEmpty)
{
    // Arrange: 受控成功回复 + 错误签名
    Stub stub;
    applyReplyStub(stub);
    g_msgType = QDBusMessage::ReplyMessage;
    g_msgSignature = QStringLiteral("a");

    // Act: 调用
    const auto result = m_tester->ListUnits();

    // Assert: 无错误且空结果
    EXPECT_EQ(result.first.getCode(), ErrorContext::kErrorTypeNoError);
    EXPECT_TRUE(result.second.empty());
}

// B3: 成功回复 a(ssssssouso) → 解析出单元列表
TEST_F(UT_Systemd1ManagerInterface, ListUnits_OnValidReply_ParsesUnitList)
{
    // Arrange: 受控成功回复 + a(ssssssouso) 载荷；读原语回灌一条记录
    Stub stub;
    applyReplyStub(stub);
    applyArgumentReadPrimitives(stub);
    g_msgType = QDBusMessage::ReplyMessage;
    g_msgSignature = QStringLiteral("a(ssssssouso)");
    g_msgArguments = makeUnitsArgs();
    g_fedAtEnd << false;            // 一轮循环
    g_fedStrings << QStringLiteral("ut.service") << QStringLiteral("UT Unit")
                 << QStringLiteral("loaded") << QStringLiteral("active")
                 << QStringLiteral("running") << QStringLiteral("-")
                 << QStringLiteral("/org/freedesktop/systemd1/unit/ut_2eservice")
                 << QStringLiteral("start")
                 << QStringLiteral("/org/freedesktop/systemd1/job/777");
    g_fedUInts << 7u;

    // Act: 调用
    const auto result = m_tester->ListUnits();

    // Assert: 无错误且记录值保持
    EXPECT_EQ(result.first.getCode(), ErrorContext::kErrorTypeNoError);
    EXPECT_EQ(result.second.size(), 1);
    if (result.second.size() == 1) {
        EXPECT_EQ(result.second.at(0).getName(), QStringLiteral("ut.service"));
        EXPECT_EQ(result.second.at(0).getLoadState(), QStringLiteral("loaded"));
        EXPECT_EQ(result.second.at(0).getActiveState(), QStringLiteral("active"));
        EXPECT_EQ(result.second.at(0).getJobId(), quint32(7));
        EXPECT_EQ(result.second.at(0).getUnitObjectPath(),
                  QStringLiteral("/org/freedesktop/systemd1/unit/ut_2eservice"));
    }
}

// ---- GetUnit/StartUnit/StopUnit/RestartUnit（TEST_P × 3 分支）----
// B1: 断连总线 → DBus 错误上下文 + 空对象路径
TEST_P(UT_Systemd1Manager_PathMethod, PathMethod_OnDisconnectedBus_SetsDBusErrorContext)
{
    const PathMethodParam param = GetParam();
    // Arrange: 未连接连接（无打桩）
    // Act: 分派调用
    const auto result = invoke(param);
    // Assert: 错误上下文为 DBus，路径为空
    EXPECT_EQ(result.first.getCode(), ErrorContext::kErrorTypeDBus) << pathMethodLabel(param.method);
    EXPECT_TRUE(result.second.path().isEmpty());
}

// B2: 成功回复但签名不符 → 路径保持空且无错误
TEST_P(UT_Systemd1Manager_PathMethod, PathMethod_OnWrongReplySignature_KeepsPathEmpty)
{
    const PathMethodParam param = GetParam();
    // Arrange: 受控成功回复 + 错误签名
    Stub stub;
    applyReplyStub(stub);
    g_msgType = QDBusMessage::ReplyMessage;
    g_msgSignature = QStringLiteral("ss");

    // Act: 分派调用
    const auto result = invoke(param);
    // Assert: 无错误且路径为空
    EXPECT_EQ(result.first.getCode(), ErrorContext::kErrorTypeNoError) << pathMethodLabel(param.method);
    EXPECT_TRUE(result.second.path().isEmpty());
}

// B3: 成功回复签名 o → 解析出对象路径
TEST_P(UT_Systemd1Manager_PathMethod, PathMethod_OnValidReply_ParsesObjectPath)
{
    const PathMethodParam param = GetParam();
    // Arrange: 受控成功回复 + 对象路径载荷
    Stub stub;
    applyReplyStub(stub);
    g_msgType = QDBusMessage::ReplyMessage;
    g_msgSignature = QStringLiteral("o");
    g_msgArguments = makeObjectPathArgs();

    // Act: 分派调用
    const auto result = invoke(param);
    // Assert: 无错误且路径保持
    EXPECT_EQ(result.first.getCode(), ErrorContext::kErrorTypeNoError) << pathMethodLabel(param.method);
    EXPECT_EQ(result.second.path(), QStringLiteral("/org/freedesktop/systemd1/job/777"));
}

// ---- GetUnitFileState ----
// B1: 断连总线 → DBus 错误上下文 + 空状态
TEST_F(UT_Systemd1ManagerInterface, GetUnitFileState_OnDisconnectedBus_SetsDBusErrorContext)
{
    // Arrange: 未连接连接（无打桩）
    // Act: 调用
    const auto result = m_tester->GetUnitFileState(QStringLiteral("ut.service"));
    // Assert: 错误上下文为 DBus 且状态为空
    EXPECT_EQ(result.first.getCode(), ErrorContext::kErrorTypeDBus);
    EXPECT_TRUE(result.second.isEmpty());
}

// B2: 成功回复但签名不符 → 状态保持空
TEST_F(UT_Systemd1ManagerInterface, GetUnitFileState_OnWrongReplySignature_KeepsStateEmpty)
{
    // Arrange: 受控成功回复 + 错误签名
    Stub stub;
    applyReplyStub(stub);
    g_msgType = QDBusMessage::ReplyMessage;
    g_msgSignature = QStringLiteral("o");

    // Act: 调用
    const auto result = m_tester->GetUnitFileState(QStringLiteral("ut.service"));

    // Assert: 无错误且状态为空
    EXPECT_EQ(result.first.getCode(), ErrorContext::kErrorTypeNoError);
    EXPECT_TRUE(result.second.isEmpty());
}

// B3: 成功回复签名 s → 解析出状态串
TEST_F(UT_Systemd1ManagerInterface, GetUnitFileState_OnValidReply_ParsesState)
{
    // Arrange: 受控成功回复 + s 载荷
    Stub stub;
    applyReplyStub(stub);
    g_msgType = QDBusMessage::ReplyMessage;
    g_msgSignature = QStringLiteral("s");
    g_msgArguments = makeStringArgs();

    // Act: 调用
    const auto result = m_tester->GetUnitFileState(QStringLiteral("ut.service"));

    // Assert: 无错误且状态保持
    EXPECT_EQ(result.first.getCode(), ErrorContext::kErrorTypeNoError);
    EXPECT_EQ(result.second, QStringLiteral("enabled"));
}

// ---- EnableUnitFiles ----
// B1: 断连总线 → DBus 错误上下文
TEST_F(UT_Systemd1ManagerInterface, EnableUnitFiles_OnDisconnectedBus_SetsDBusErrorContext)
{
    // Arrange: 未连接连接（无打桩）
    // Act: 调用（默认 runtime/force）
    const auto result = m_tester->EnableUnitFiles(QStringList() << QStringLiteral("ut.service"));
    // Assert: 错误上下文为 DBus
    EXPECT_EQ(result.first.getCode(), ErrorContext::kErrorTypeDBus);
    EXPECT_EQ(result.first.getErrorName(), QStringLiteral("org.freedesktop.DBus.Error.Disconnected"));
}

// B2/B4: 成功回复 → TODO 空实现，结果保持默认
TEST_F(UT_Systemd1ManagerInterface, EnableUnitFiles_OnValidReply_KeepsDefaultResult)
{
    // Arrange: 受控成功回复（成功分支为 TODO，不解析）
    Stub stub;
    applyReplyStub(stub);
    g_msgType = QDBusMessage::ReplyMessage;
    g_msgSignature = QStringLiteral("ba(sss)");
    g_msgArguments.clear();

    // Act: 调用
    const auto result = m_tester->EnableUnitFiles(QStringList() << QStringLiteral("ut.service"),
                                                  true, false);
    // Assert: 无错误且结果为默认构造
    EXPECT_EQ(result.first.getCode(), ErrorContext::kErrorTypeNoError);
    EXPECT_FALSE(result.second.hasInstallInfo);
    EXPECT_TRUE(result.second.changes.isEmpty());
}

// ---- DisableUnitFiles ----
// B1: 断连总线 → DBus 错误上下文
TEST_F(UT_Systemd1ManagerInterface, DisableUnitFiles_OnDisconnectedBus_SetsDBusErrorContext)
{
    // Arrange: 未连接连接（无打桩）
    // Act: 调用（默认 runtime）
    const auto result = m_tester->DisableUnitFiles(QStringList() << QStringLiteral("ut.service"));
    // Assert: 错误上下文为 DBus
    EXPECT_EQ(result.first.getCode(), ErrorContext::kErrorTypeDBus);
    EXPECT_EQ(result.first.getErrorName(), QStringLiteral("org.freedesktop.DBus.Error.Disconnected"));
}

// B2/B4: 成功回复 → TODO 空实现，结果保持默认
TEST_F(UT_Systemd1ManagerInterface, DisableUnitFiles_OnValidReply_KeepsDefaultResult)
{
    // Arrange: 受控成功回复（成功分支为 TODO，不解析）
    Stub stub;
    applyReplyStub(stub);
    g_msgType = QDBusMessage::ReplyMessage;
    g_msgSignature = QStringLiteral("a(sss)");
    g_msgArguments.clear();

    // Act: 调用
    const auto result = m_tester->DisableUnitFiles(QStringList() << QStringLiteral("ut.service"),
                                                   false);
    // Assert: 无错误且结果为默认构造
    EXPECT_EQ(result.first.getCode(), ErrorContext::kErrorTypeNoError);
    EXPECT_TRUE(result.second.changes.isEmpty());
}
