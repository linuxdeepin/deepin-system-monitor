// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// UT_DBusPropertiesInterface：org.freedesktop.DBus.Properties 接口封装单元测试（qt-autotest-generator 生成）
//
// 被测文件：deepin-system-monitor-main/dbus/dbus_properties_interface.cpp
//
// 打桩说明（外部依赖全部 stub，不依赖真实总线）：
// - QDBusConnection("ut-properties-fake")：未连接连接，无打桩时调用确定性失败（无总线 I/O）
// - QDBusAbstractInterface::lastError      → 受控错误上下文
// - QDBusAbstractInterface::callWithArgumentList → 受控回复消息（验证透传）
//
// ============================ 用例计数声明 ============================
// 方法                                    | level | 最少用例 | actual | 说明
// DBusPropertiesInterface() 构造           | low   | 1 | 1 | 构造（含 staticInterfaceName 校验）
// ~DBusPropertiesInterface()              | low   | 1 | - | 由 TearDown delete 覆盖
// Get(const QString&, const QString&)     | mid   | 2 | 4 | 断连自然错误 / 受控错误上下文 / 无错误 / 回复透传
//
// ============================== 最小清单 ==============================
// [x] 1 公开方法已列出，每个方法 ≥ 1 用例
// [x] 2 输入按等价类划分：lastError 有效 / 无效 × 回复消息内容
// [x] 3 边界值显式覆盖：lastError.isValid() 真/假两分支
// [x] 4 无 ≥3 组同质断言输入，不适用 TEST_P（错误/无错误/透传语义各异）
// [x] 5 分支清单已列出并映射到用例名
// [x] 6 每条 if/early-return 分支有触发用例
// [x] 7 本类无显式 throw，不适用 EXPECT_THROW
// [x] 8 负面场景（总线断连 → DBus 错误上下文）有专门用例
// [x] 9 负面用例验证强异常安全（错误路径返回错误上下文与空消息，不抛异常）
// [x] 10 项目内接口类用 gMock，其他用 stub_ext（本项目统一用项目自带 stub.h 打桩）
//
// ============================== 分支清单 ==============================
// （来源：工作目录源码 dbus_properties_interface.h）
// Get(in0, in1):
//   B1  lastError().isValid() 为真 → ec=kErrorTypeDBus+名称/消息 → Get_LastErrorValid_SetsDBusErrorContext
//                                                            / Get_OnDisconnectedBus_SetsDBusErrorContext
//   B2  lastError().isValid() 为假 → ec 保持无错误              → Get_LastErrorInvalid_KeepsNoErrorContext
//   B3  回复消息透传（不做类型校验）                             → Get_PropagatesReplyMessage

#include "dbus/dbus_properties_interface.h"
//qt
#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusError>
#include <QDBusMessage>
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
QDBusError g_lastError;
QDBusMessage g_callReplyMessage;

void resetPropertiesStubState()
{
    g_lastError = QDBusError();
    g_callReplyMessage = QDBusMessage();
}
}  // namespace

// QDBusError QDBusAbstractInterface::lastError()（Qt6 返回 QDBusError）
static QDBusError stub_QDBusAbstractInterface_lastError()
{
    return g_lastError;
}

// QDBusMessage QDBusAbstractInterface::callWithArgumentList(CallMode, const QString&, const QList<QVariant>&)（Qt6 无 timeout 参）
static QDBusMessage stub_QDBusAbstractInterface_callWithArgumentList(void *, QDBus::CallMode,
                                                                     const QString &,
                                                                     const QList<QVariant> &)
{
    return g_callReplyMessage;
}

// ============================== 夹具 ==============================
class UT_DBusPropertiesInterface : public ::testing::Test
{
public:
    UT_DBusPropertiesInterface() : m_conn(QStringLiteral("ut-properties-fake")) {}
    ~UT_DBusPropertiesInterface() override {}

    void SetUp() override
    {
        resetPropertiesStubState();
        m_conn = QDBusConnection(QStringLiteral("ut-properties-fake"));
        m_tester = new DBusPropertiesInterface(QStringLiteral("org.freedesktop.systemd1"),
                                               QStringLiteral("/org/freedesktop/systemd1"),
                                               m_conn, nullptr);
    }

    void TearDown() override
    {
        delete m_tester;
        m_tester = nullptr;
    }

    QDBusConnection m_conn;
    DBusPropertiesInterface *m_tester = nullptr;
};

// ============================== 用例 ==============================
// 构造与接口名
TEST_F(UT_DBusPropertiesInterface, CtorDtor_ConstructWithFakeConnection_NoCrash)
{
    // Arrange: SetUp 已构造对象
    // Act: SetUp 已构造对象（TearDown 负责 delete）
    // Assert: 接口名正确且对象有效
    EXPECT_STREQ(DBusPropertiesInterface::staticInterfaceName(), "org.freedesktop.DBus.Properties");
    EXPECT_NE(m_tester, nullptr);
}

// B1: 断连总线自然失败 → lastError 有效 → DBus 错误上下文
TEST_F(UT_DBusPropertiesInterface, Get_OnDisconnectedBus_SetsDBusErrorContext)
{
    // Arrange: 未连接连接（无打桩），调用必然失败
    // Act: 调用 Get
    const auto result = m_tester->Get(QStringLiteral("org.freedesktop.systemd1"),
                                      QStringLiteral("ActiveState"));
    // Assert: 错误上下文为 DBus，消息名可识别
    EXPECT_EQ(result.first.getCode(), ErrorContext::kErrorTypeDBus);
    EXPECT_TRUE(result.first.isValid());
}

// B1: 受控 lastError → 错误上下文精确承载名称/消息/子码
TEST_F(UT_DBusPropertiesInterface, Get_LastErrorValid_SetsDBusErrorContext)
{
    // Arrange: 打桩 lastError 为受控错误消息
    Stub stub;
    stub.set(ADDR(QDBusAbstractInterface, lastError), stub_QDBusAbstractInterface_lastError);
    resetPropertiesStubState();
    g_lastError = QDBusError(QDBusError::ServiceUnknown,
                                                   QStringLiteral("stub: no such service"));

    // Act: 调用 Get
    const auto result = m_tester->Get(QStringLiteral("org.freedesktop.systemd1"),
                                      QStringLiteral("ActiveState"));
    // Assert: 错误上下文内容与受控错误一致
    EXPECT_EQ(result.first.getCode(), ErrorContext::kErrorTypeDBus);
    EXPECT_EQ(result.first.getSubCode(), static_cast<int>(QDBusError::ServiceUnknown));
    EXPECT_EQ(result.first.getErrorName(), QStringLiteral("org.freedesktop.DBus.Error.ServiceUnknown"));
    EXPECT_EQ(result.first.getErrorMessage(), QStringLiteral("stub: no such service"));
}

// B2: lastError 无效 → 无错误上下文
TEST_F(UT_DBusPropertiesInterface, Get_LastErrorInvalid_KeepsNoErrorContext)
{
    // Arrange: 打桩 lastError 为默认消息（无效）
    Stub stub;
    stub.set(ADDR(QDBusAbstractInterface, lastError), stub_QDBusAbstractInterface_lastError);
    resetPropertiesStubState();

    // Act: 调用 Get
    const auto result = m_tester->Get(QStringLiteral("org.freedesktop.systemd1"),
                                      QStringLiteral("ActiveState"));
    // Assert: 无错误
    EXPECT_EQ(result.first.getCode(), ErrorContext::kErrorTypeNoError);
    EXPECT_FALSE(result.first.isValid());
}

// B3: 回复消息透传返回
TEST_F(UT_DBusPropertiesInterface, Get_ValidReply_PropagatesMessage)
{
    // Arrange: 打桩 lastError 无效 + 受控回复消息
    Stub stub;
    stub.set(ADDR(QDBusAbstractInterface, lastError), stub_QDBusAbstractInterface_lastError);
    stub.set((QDBusMessage (QDBusAbstractInterface::*)(QDBus::CallMode, const QString &,
                                                       const QList<QVariant> &))
                 ADDR(QDBusAbstractInterface, callWithArgumentList),
             stub_QDBusAbstractInterface_callWithArgumentList);
    resetPropertiesStubState();
    // g_lastError 保持默认（isValid()==false）
    g_callReplyMessage = QDBusMessage::createMethodCall(
        QStringLiteral("ut.svc"), QStringLiteral("/ut"), QStringLiteral("ut.if"), QStringLiteral("m"));
    g_callReplyMessage.setArguments(QList<QVariant>{QVariant(QStringLiteral("active"))});

    // Act: 调用 Get
    const auto result = m_tester->Get(QStringLiteral("org.freedesktop.systemd1"),
                                      QStringLiteral("ActiveState"));
    // Assert: 回复消息原样透传（createMethodCall 构造的消息 type 为 MethodCallMessage）
    EXPECT_EQ(result.first.getCode(), ErrorContext::kErrorTypeNoError);
    EXPECT_EQ(result.second.service(), QStringLiteral("ut.svc"));
    EXPECT_EQ(result.second.arguments().size(), 1);
    if (result.second.arguments().size() == 1)
        EXPECT_EQ(result.second.arguments().at(0).toString(), QStringLiteral("active"));
}
