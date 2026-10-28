// SPDX-FileCopyrightText: 2019-2026 UnionTech Software Technology Co., Ltd.
// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// 用例计数声明（self-check-structural 验证此块）：
// | method        | level | factors | min | actual |
// |---------------|-------|---------|-----|--------|
// | ctor          | low   | -       | 1   | 1      |
// | getId         | low   | -       | 1   | 4      |
// | getLoadState  | low   | -       | 1   | 4      |
// | getActiveState| low   | -       | 1   | 4      |
// | getSubState   | low   | -       | 1   | 4      |
// | getDescription| low   | -       | 1   | 4      |
// | canReload     | low   | -       | 1   | 5      |
// | canStart      | low   | -       | 1   | 5      |
// | canStop       | low   | -       | 1   | 5      |
// | dtor          | low   | -       | 1   | 全局隐式覆盖 |

//self
#include "dbus/systemd1_unit_interface.h"

//被桩对象（属性接口）
#include "dbus/dbus_properties_interface.h"
#include "common/error_context.h"

//gtest
#include "stub.h"
#include <gtest/gtest.h>

//qt
#include <QDBusMessage>
#include <QDBusVariant>
#include <QDBusError>
#include <QMap>
#include <QVariant>

/***************************************STUB begin*********************************************/
// 桩控状态：property -> variant 值；g_propertyError 打开时优先返回 DBus 错误上下文。
// 每个用例在 fixture SetUp 中重置，保证用例隔离。
static QMap<QString, QVariant> g_propertyValues;
static bool g_propertyError = false;

// 构造一条"应答"消息：createReply/createErrorReply 是非静态成员，
// 需先造一条调用消息再以其为基准构造应答。
static QDBusMessage makeReplyMessage(const QVariant &argument)
{
    QDBusMessage call = QDBusMessage::createMethodCall(
        QStringLiteral("ut.stub"), QStringLiteral("/ut/stub"),
        QStringLiteral("ut.stub.iface"), QStringLiteral("get"));
    return call.createReply(argument);
}

static QDBusMessage makeErrorMessage(QDBusError::ErrorType type, const QString &message)
{
    QDBusMessage call = QDBusMessage::createMethodCall(
        QStringLiteral("ut.stub"), QStringLiteral("/ut/stub"),
        QStringLiteral("ut.stub.iface"), QStringLiteral("get"));
    return call.createErrorReply(type, message);
}

// 桩 DBusPropertiesInterface::Get：不发真实 DBus 调用，返回预构造的应答
static QPair<ErrorContext, QDBusMessage> stub_propertyGet(DBusPropertiesInterface *self, const QString &interface, const QString &property)
{
    Q_UNUSED(self);
    Q_UNUSED(interface);

    ErrorContext ec;
    QDBusMessage msg;

    // 场景一：模拟底层 DBus 调用失败（lastError 有效）
    if (g_propertyError) {
        ec.setCode(ErrorContext::kErrorTypeDBus);
        ec.setSubCode(static_cast<int>(QDBusError::InvalidArgs));
        ec.setErrorName(QStringLiteral("org.freedesktop.DBus.Error.InvalidArgs"));
        ec.setErrorMessage(QStringLiteral("stub dbus error"));
        return qMakePair(ec, msg);
    }

    // 场景二：属性未注入，模拟 systemd 返回 ErrorMessage 应答
    if (!g_propertyValues.contains(property)) {
        msg = makeErrorMessage(QDBusError::UnknownProperty,
                               QStringLiteral("stub: unknown property"));
        return qMakePair(ec, msg);
    }

    // 场景三：正常应答，ReplyMessage + QDBusVariant 参数
    msg = makeReplyMessage(
        QVariant::fromValue(QDBusVariant(g_propertyValues.value(property))));
    return qMakePair(ec, msg);
}
/***************************************STUB end***********************************************/

class UT_Systemd1UnitInterface: public ::testing::Test
{
public:
    UT_Systemd1UnitInterface() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        // Arrange: 重置桩控状态并构造被测接口（不连接真实 DBus，Get 已被桩替换）
        g_propertyValues.clear();
        g_propertyError = false;
        static QDBusConnection fakeConn("ut-sm-fake-session");
        m_tester = new Systemd1UnitInterface(QStringLiteral("org.freedesktop.systemd1"),
                                             QStringLiteral("/org/freedesktop/systemd1/unit/dbus_2eservice"),
                                             fakeConn);
    }

    virtual void TearDown()
    {
        if (m_tester) {
            delete m_tester;
            m_tester = nullptr;
        }
    }

protected:
    Systemd1UnitInterface *m_tester;
};

// ─────────────────────────── 构造 / 析构 ───────────────────────────

TEST_F(UT_Systemd1UnitInterface, Ctor_ValidParams_InterfaceConfigured)
{
    // Arrange: SetUp 已用合法 service/path 构造对象
    // Act: 读取接口基础信息
    const QString service = m_tester->service();
    const QString path = m_tester->path();

    // Assert: service/path/interface 均与构造参数一致
    EXPECT_EQ(service, QStringLiteral("org.freedesktop.systemd1"));
    EXPECT_EQ(path, QStringLiteral("/org/freedesktop/systemd1/unit/dbus_2eservice"));
    EXPECT_EQ(m_tester->interface(), QString(Systemd1UnitInterface::staticInterfaceName()));
}

// ─────────────────────────── getId ───────────────────────────

TEST_F(UT_Systemd1UnitInterface, GetId_ReplyString_ReturnsId)
{
    // Arrange: 注入字符串属性 Id，并挂上属性桩
    Stub stub;
    g_propertyValues.insert("Id", QStringLiteral("dbus.service"));
    stub.set(ADDR(DBusPropertiesInterface, Get), stub_propertyGet);

    // Act: 调用 getId
    auto ret = m_tester->getId();

    // Assert: 无错误且取回注入的 Id
    EXPECT_FALSE(ret.first.isValid());
    EXPECT_EQ(ret.first.getCode(), ErrorContext::kErrorTypeNoError);
    EXPECT_EQ(ret.second, QStringLiteral("dbus.service"));
}

TEST_F(UT_Systemd1UnitInterface, GetId_DbusError_ReturnsErrorContextAndEmptyId)
{
    // Arrange: 打开桩的 DBus 错误开关
    Stub stub;
    g_propertyError = true;
    stub.set(ADDR(DBusPropertiesInterface, Get), stub_propertyGet);

    // Act: 调用 getId
    auto ret = m_tester->getId();

    // Assert: 返回错误上下文与空 Id
    EXPECT_TRUE(ret.first.isValid());
    EXPECT_EQ(ret.first.getCode(), ErrorContext::kErrorTypeDBus);
    EXPECT_TRUE(ret.second.isEmpty());
}

TEST_F(UT_Systemd1UnitInterface, GetId_ErrorMessageReply_ReturnsNoErrorAndEmptyId)
{
    // Arrange: 不注入属性 -> 桩返回 ErrorMessage 类型应答
    Stub stub;
    stub.set(ADDR(DBusPropertiesInterface, Get), stub_propertyGet);

    // Act: 调用 getId
    auto ret = m_tester->getId();

    // Assert: ErrorContext 保持无错误，Id 为默认空值
    EXPECT_FALSE(ret.first.isValid());
    EXPECT_EQ(ret.first.getCode(), ErrorContext::kErrorTypeNoError);
    EXPECT_TRUE(ret.second.isEmpty());
}

TEST_F(UT_Systemd1UnitInterface, GetId_WrongTypeReply_ReturnsEmptyId)
{
    // Arrange: 注入整型 Id（类型不符）
    Stub stub;
    g_propertyValues.insert("Id", 12345);
    stub.set(ADDR(DBusPropertiesInterface, Get), stub_propertyGet);

    // Act: 调用 getId
    auto ret = m_tester->getId();

    // Assert: 类型不符时返回默认空 Id，无错误
    EXPECT_FALSE(ret.first.isValid());
    EXPECT_EQ(ret.first.getCode(), ErrorContext::kErrorTypeNoError);
    EXPECT_TRUE(ret.second.isEmpty());
}

// ─────────────────────────── getLoadState ───────────────────────────

TEST_F(UT_Systemd1UnitInterface, GetLoadState_ReplyString_ReturnsLoadState)
{
    // Arrange: 注入字符串属性 LoadState
    Stub stub;
    g_propertyValues.insert("LoadState", QStringLiteral("loaded"));
    stub.set(ADDR(DBusPropertiesInterface, Get), stub_propertyGet);

    // Act: 调用 getLoadState
    auto ret = m_tester->getLoadState();

    // Assert: 无错误且取回注入值
    EXPECT_FALSE(ret.first.isValid());
    EXPECT_EQ(ret.first.getCode(), ErrorContext::kErrorTypeNoError);
    EXPECT_EQ(ret.second, QStringLiteral("loaded"));
}

TEST_F(UT_Systemd1UnitInterface, GetLoadState_DbusError_ReturnsErrorContextAndEmptyState)
{
    // Arrange: 打开桩的 DBus 错误开关
    Stub stub;
    g_propertyError = true;
    stub.set(ADDR(DBusPropertiesInterface, Get), stub_propertyGet);

    // Act: 调用 getLoadState
    auto ret = m_tester->getLoadState();

    // Assert: 返回错误上下文与空状态
    EXPECT_TRUE(ret.first.isValid());
    EXPECT_EQ(ret.first.getCode(), ErrorContext::kErrorTypeDBus);
    EXPECT_TRUE(ret.second.isEmpty());
}

TEST_F(UT_Systemd1UnitInterface, GetLoadState_ErrorMessageReply_ReturnsNoErrorAndEmptyState)
{
    // Arrange: 不注入属性 -> 桩返回 ErrorMessage 应答
    Stub stub;
    stub.set(ADDR(DBusPropertiesInterface, Get), stub_propertyGet);

    // Act: 调用 getLoadState
    auto ret = m_tester->getLoadState();

    // Assert: 无错误上下文，状态为默认空值
    EXPECT_FALSE(ret.first.isValid());
    EXPECT_EQ(ret.first.getCode(), ErrorContext::kErrorTypeNoError);
    EXPECT_TRUE(ret.second.isEmpty());
}

TEST_F(UT_Systemd1UnitInterface, GetLoadState_WrongTypeReply_ReturnsEmptyState)
{
    // Arrange: 注入布尔 LoadState（类型不符）
    Stub stub;
    g_propertyValues.insert("LoadState", true);
    stub.set(ADDR(DBusPropertiesInterface, Get), stub_propertyGet);

    // Act: 调用 getLoadState
    auto ret = m_tester->getLoadState();

    // Assert: 类型不符返回默认空状态
    EXPECT_FALSE(ret.first.isValid());
    EXPECT_EQ(ret.first.getCode(), ErrorContext::kErrorTypeNoError);
    EXPECT_TRUE(ret.second.isEmpty());
}

// ─────────────────────────── getActiveState ───────────────────────────

TEST_F(UT_Systemd1UnitInterface, GetActiveState_ReplyString_ReturnsActiveState)
{
    // Arrange: 注入字符串属性 ActiveState
    Stub stub;
    g_propertyValues.insert("ActiveState", QStringLiteral("active"));
    stub.set(ADDR(DBusPropertiesInterface, Get), stub_propertyGet);

    // Act: 调用 getActiveState
    auto ret = m_tester->getActiveState();

    // Assert: 无错误且取回注入值
    EXPECT_FALSE(ret.first.isValid());
    EXPECT_EQ(ret.first.getCode(), ErrorContext::kErrorTypeNoError);
    EXPECT_EQ(ret.second, QStringLiteral("active"));
}

TEST_F(UT_Systemd1UnitInterface, GetActiveState_DbusError_ReturnsErrorContextAndEmptyState)
{
    // Arrange: 打开桩的 DBus 错误开关
    Stub stub;
    g_propertyError = true;
    stub.set(ADDR(DBusPropertiesInterface, Get), stub_propertyGet);

    // Act: 调用 getActiveState
    auto ret = m_tester->getActiveState();

    // Assert: 返回错误上下文与空状态
    EXPECT_TRUE(ret.first.isValid());
    EXPECT_EQ(ret.first.getCode(), ErrorContext::kErrorTypeDBus);
    EXPECT_TRUE(ret.second.isEmpty());
}

TEST_F(UT_Systemd1UnitInterface, GetActiveState_ErrorMessageReply_ReturnsNoErrorAndEmptyState)
{
    // Arrange: 不注入属性 -> 桩返回 ErrorMessage 应答
    Stub stub;
    stub.set(ADDR(DBusPropertiesInterface, Get), stub_propertyGet);

    // Act: 调用 getActiveState
    auto ret = m_tester->getActiveState();

    // Assert: 无错误上下文，状态为默认空值
    EXPECT_FALSE(ret.first.isValid());
    EXPECT_EQ(ret.first.getCode(), ErrorContext::kErrorTypeNoError);
    EXPECT_TRUE(ret.second.isEmpty());
}

TEST_F(UT_Systemd1UnitInterface, GetActiveState_WrongTypeReply_ReturnsEmptyState)
{
    // Arrange: 注入整型 ActiveState（类型不符）
    Stub stub;
    g_propertyValues.insert("ActiveState", 1);
    stub.set(ADDR(DBusPropertiesInterface, Get), stub_propertyGet);

    // Act: 调用 getActiveState
    auto ret = m_tester->getActiveState();

    // Assert: 类型不符返回默认空状态
    EXPECT_FALSE(ret.first.isValid());
    EXPECT_EQ(ret.first.getCode(), ErrorContext::kErrorTypeNoError);
    EXPECT_TRUE(ret.second.isEmpty());
}

// ─────────────────────────── getSubState ───────────────────────────

TEST_F(UT_Systemd1UnitInterface, GetSubState_ReplyString_ReturnsSubState)
{
    // Arrange: 注入字符串属性 SubState
    Stub stub;
    g_propertyValues.insert("SubState", QStringLiteral("running"));
    stub.set(ADDR(DBusPropertiesInterface, Get), stub_propertyGet);

    // Act: 调用 getSubState
    auto ret = m_tester->getSubState();

    // Assert: 无错误且取回注入值
    EXPECT_FALSE(ret.first.isValid());
    EXPECT_EQ(ret.first.getCode(), ErrorContext::kErrorTypeNoError);
    EXPECT_EQ(ret.second, QStringLiteral("running"));
}

TEST_F(UT_Systemd1UnitInterface, GetSubState_DbusError_ReturnsErrorContextAndEmptyState)
{
    // Arrange: 打开桩的 DBus 错误开关
    Stub stub;
    g_propertyError = true;
    stub.set(ADDR(DBusPropertiesInterface, Get), stub_propertyGet);

    // Act: 调用 getSubState
    auto ret = m_tester->getSubState();

    // Assert: 返回错误上下文与空状态
    EXPECT_TRUE(ret.first.isValid());
    EXPECT_EQ(ret.first.getCode(), ErrorContext::kErrorTypeDBus);
    EXPECT_TRUE(ret.second.isEmpty());
}

TEST_F(UT_Systemd1UnitInterface, GetSubState_ErrorMessageReply_ReturnsNoErrorAndEmptyState)
{
    // Arrange: 不注入属性 -> 桩返回 ErrorMessage 应答
    Stub stub;
    stub.set(ADDR(DBusPropertiesInterface, Get), stub_propertyGet);

    // Act: 调用 getSubState
    auto ret = m_tester->getSubState();

    // Assert: 无错误上下文，状态为默认空值
    EXPECT_FALSE(ret.first.isValid());
    EXPECT_EQ(ret.first.getCode(), ErrorContext::kErrorTypeNoError);
    EXPECT_TRUE(ret.second.isEmpty());
}

TEST_F(UT_Systemd1UnitInterface, GetSubState_WrongTypeReply_ReturnsEmptyState)
{
    // Arrange: 注入整型 SubState（类型不符）
    Stub stub;
    g_propertyValues.insert("SubState", 2);
    stub.set(ADDR(DBusPropertiesInterface, Get), stub_propertyGet);

    // Act: 调用 getSubState
    auto ret = m_tester->getSubState();

    // Assert: 类型不符返回默认空状态
    EXPECT_FALSE(ret.first.isValid());
    EXPECT_EQ(ret.first.getCode(), ErrorContext::kErrorTypeNoError);
    EXPECT_TRUE(ret.second.isEmpty());
}

// ─────────────────────────── getDescription ───────────────────────────

TEST_F(UT_Systemd1UnitInterface, GetDescription_ReplyString_ReturnsDescription)
{
    // Arrange: 注入字符串属性 Description
    Stub stub;
    g_propertyValues.insert("Description", QStringLiteral("D-Bus System Service"));
    stub.set(ADDR(DBusPropertiesInterface, Get), stub_propertyGet);

    // Act: 调用 getDescription
    auto ret = m_tester->getDescription();

    // Assert: 无错误且取回注入值
    EXPECT_FALSE(ret.first.isValid());
    EXPECT_EQ(ret.first.getCode(), ErrorContext::kErrorTypeNoError);
    EXPECT_EQ(ret.second, QStringLiteral("D-Bus System Service"));
}

TEST_F(UT_Systemd1UnitInterface, GetDescription_DbusError_ReturnsErrorContextAndEmptyDescription)
{
    // Arrange: 打开桩的 DBus 错误开关
    Stub stub;
    g_propertyError = true;
    stub.set(ADDR(DBusPropertiesInterface, Get), stub_propertyGet);

    // Act: 调用 getDescription
    auto ret = m_tester->getDescription();

    // Assert: 返回错误上下文与空描述
    EXPECT_TRUE(ret.first.isValid());
    EXPECT_EQ(ret.first.getCode(), ErrorContext::kErrorTypeDBus);
    EXPECT_TRUE(ret.second.isEmpty());
}

TEST_F(UT_Systemd1UnitInterface, GetDescription_ErrorMessageReply_ReturnsNoErrorAndEmptyDescription)
{
    // Arrange: 不注入属性 -> 桩返回 ErrorMessage 应答
    Stub stub;
    stub.set(ADDR(DBusPropertiesInterface, Get), stub_propertyGet);

    // Act: 调用 getDescription
    auto ret = m_tester->getDescription();

    // Assert: 无错误上下文，描述为默认空值
    EXPECT_FALSE(ret.first.isValid());
    EXPECT_EQ(ret.first.getCode(), ErrorContext::kErrorTypeNoError);
    EXPECT_TRUE(ret.second.isEmpty());
}

TEST_F(UT_Systemd1UnitInterface, GetDescription_WrongTypeReply_ReturnsEmptyDescription)
{
    // Arrange: 注入布尔 Description（类型不符）
    Stub stub;
    g_propertyValues.insert("Description", true);
    stub.set(ADDR(DBusPropertiesInterface, Get), stub_propertyGet);

    // Act: 调用 getDescription
    auto ret = m_tester->getDescription();

    // Assert: 类型不符返回默认空描述
    EXPECT_FALSE(ret.first.isValid());
    EXPECT_EQ(ret.first.getCode(), ErrorContext::kErrorTypeNoError);
    EXPECT_TRUE(ret.second.isEmpty());
}

// ─────────────────────────── canReload ───────────────────────────

TEST_F(UT_Systemd1UnitInterface, CanReload_ReplyTrue_ReturnsTrue)
{
    // Arrange: 注入布尔属性 CanReload = true
    Stub stub;
    g_propertyValues.insert("CanReload", true);
    stub.set(ADDR(DBusPropertiesInterface, Get), stub_propertyGet);

    // Act: 调用 canReload
    auto ret = m_tester->canReload();

    // Assert: 无错误且返回 true
    EXPECT_FALSE(ret.first.isValid());
    EXPECT_EQ(ret.first.getCode(), ErrorContext::kErrorTypeNoError);
    EXPECT_TRUE(ret.second);
}

TEST_F(UT_Systemd1UnitInterface, CanReload_ReplyFalse_ReturnsFalse)
{
    // Arrange: 注入布尔属性 CanReload = false
    Stub stub;
    g_propertyValues.insert("CanReload", false);
    stub.set(ADDR(DBusPropertiesInterface, Get), stub_propertyGet);

    // Act: 调用 canReload
    auto ret = m_tester->canReload();

    // Assert: 无错误且返回 false
    EXPECT_FALSE(ret.first.isValid());
    EXPECT_EQ(ret.first.getCode(), ErrorContext::kErrorTypeNoError);
    EXPECT_FALSE(ret.second);
}

TEST_F(UT_Systemd1UnitInterface, CanReload_DbusError_ReturnsErrorContextAndFalse)
{
    // Arrange: 打开桩的 DBus 错误开关
    Stub stub;
    g_propertyError = true;
    stub.set(ADDR(DBusPropertiesInterface, Get), stub_propertyGet);

    // Act: 调用 canReload
    auto ret = m_tester->canReload();

    // Assert: 返回错误上下文与默认 false
    EXPECT_TRUE(ret.first.isValid());
    EXPECT_EQ(ret.first.getCode(), ErrorContext::kErrorTypeDBus);
    EXPECT_FALSE(ret.second);
}

TEST_F(UT_Systemd1UnitInterface, CanReload_ErrorMessageReply_ReturnsNoErrorAndFalse)
{
    // Arrange: 不注入属性 -> 桩返回 ErrorMessage 应答
    Stub stub;
    stub.set(ADDR(DBusPropertiesInterface, Get), stub_propertyGet);

    // Act: 调用 canReload
    auto ret = m_tester->canReload();

    // Assert: 无错误上下文，返回默认 false
    EXPECT_FALSE(ret.first.isValid());
    EXPECT_EQ(ret.first.getCode(), ErrorContext::kErrorTypeNoError);
    EXPECT_FALSE(ret.second);
}

TEST_F(UT_Systemd1UnitInterface, CanReload_WrongTypeReply_ReturnsFalse)
{
    // Arrange: 注入字符串 CanReload（类型不符）
    Stub stub;
    g_propertyValues.insert("CanReload", QStringLiteral("yes"));
    stub.set(ADDR(DBusPropertiesInterface, Get), stub_propertyGet);

    // Act: 调用 canReload
    auto ret = m_tester->canReload();

    // Assert: 类型不符返回默认 false
    EXPECT_FALSE(ret.first.isValid());
    EXPECT_EQ(ret.first.getCode(), ErrorContext::kErrorTypeNoError);
    EXPECT_FALSE(ret.second);
}

// ─────────────────────────── canStart ───────────────────────────

TEST_F(UT_Systemd1UnitInterface, CanStart_ReplyTrue_ReturnsTrue)
{
    // Arrange: 注入布尔属性 CanStart = true
    Stub stub;
    g_propertyValues.insert("CanStart", true);
    stub.set(ADDR(DBusPropertiesInterface, Get), stub_propertyGet);

    // Act: 调用 canStart
    auto ret = m_tester->canStart();

    // Assert: 无错误且返回 true
    EXPECT_FALSE(ret.first.isValid());
    EXPECT_EQ(ret.first.getCode(), ErrorContext::kErrorTypeNoError);
    EXPECT_TRUE(ret.second);
}

TEST_F(UT_Systemd1UnitInterface, CanStart_ReplyFalse_ReturnsFalse)
{
    // Arrange: 注入布尔属性 CanStart = false
    Stub stub;
    g_propertyValues.insert("CanStart", false);
    stub.set(ADDR(DBusPropertiesInterface, Get), stub_propertyGet);

    // Act: 调用 canStart
    auto ret = m_tester->canStart();

    // Assert: 无错误且返回 false
    EXPECT_FALSE(ret.first.isValid());
    EXPECT_EQ(ret.first.getCode(), ErrorContext::kErrorTypeNoError);
    EXPECT_FALSE(ret.second);
}

TEST_F(UT_Systemd1UnitInterface, CanStart_DbusError_ReturnsErrorContextAndFalse)
{
    // Arrange: 打开桩的 DBus 错误开关
    Stub stub;
    g_propertyError = true;
    stub.set(ADDR(DBusPropertiesInterface, Get), stub_propertyGet);

    // Act: 调用 canStart
    auto ret = m_tester->canStart();

    // Assert: 返回错误上下文与默认 false
    EXPECT_TRUE(ret.first.isValid());
    EXPECT_EQ(ret.first.getCode(), ErrorContext::kErrorTypeDBus);
    EXPECT_FALSE(ret.second);
}

TEST_F(UT_Systemd1UnitInterface, CanStart_ErrorMessageReply_ReturnsNoErrorAndFalse)
{
    // Arrange: 不注入属性 -> 桩返回 ErrorMessage 应答
    Stub stub;
    stub.set(ADDR(DBusPropertiesInterface, Get), stub_propertyGet);

    // Act: 调用 canStart
    auto ret = m_tester->canStart();

    // Assert: 无错误上下文，返回默认 false
    EXPECT_FALSE(ret.first.isValid());
    EXPECT_EQ(ret.first.getCode(), ErrorContext::kErrorTypeNoError);
    EXPECT_FALSE(ret.second);
}

TEST_F(UT_Systemd1UnitInterface, CanStart_WrongTypeReply_ReturnsFalse)
{
    // Arrange: 注入整型 CanStart（类型不符）
    Stub stub;
    g_propertyValues.insert("CanStart", 1);
    stub.set(ADDR(DBusPropertiesInterface, Get), stub_propertyGet);

    // Act: 调用 canStart
    auto ret = m_tester->canStart();

    // Assert: 类型不符返回默认 false
    EXPECT_FALSE(ret.first.isValid());
    EXPECT_EQ(ret.first.getCode(), ErrorContext::kErrorTypeNoError);
    EXPECT_FALSE(ret.second);
}

// ─────────────────────────── canStop ───────────────────────────

TEST_F(UT_Systemd1UnitInterface, CanStop_ReplyTrue_ReturnsTrue)
{
    // Arrange: 注入布尔属性 CanStop = true
    Stub stub;
    g_propertyValues.insert("CanStop", true);
    stub.set(ADDR(DBusPropertiesInterface, Get), stub_propertyGet);

    // Act: 调用 canStop
    auto ret = m_tester->canStop();

    // Assert: 无错误且返回 true
    EXPECT_FALSE(ret.first.isValid());
    EXPECT_EQ(ret.first.getCode(), ErrorContext::kErrorTypeNoError);
    EXPECT_TRUE(ret.second);
}

TEST_F(UT_Systemd1UnitInterface, CanStop_ReplyFalse_ReturnsFalse)
{
    // Arrange: 注入布尔属性 CanStop = false
    Stub stub;
    g_propertyValues.insert("CanStop", false);
    stub.set(ADDR(DBusPropertiesInterface, Get), stub_propertyGet);

    // Act: 调用 canStop
    auto ret = m_tester->canStop();

    // Assert: 无错误且返回 false
    EXPECT_FALSE(ret.first.isValid());
    EXPECT_EQ(ret.first.getCode(), ErrorContext::kErrorTypeNoError);
    EXPECT_FALSE(ret.second);
}

TEST_F(UT_Systemd1UnitInterface, CanStop_DbusError_ReturnsErrorContextAndFalse)
{
    // Arrange: 打开桩的 DBus 错误开关
    Stub stub;
    g_propertyError = true;
    stub.set(ADDR(DBusPropertiesInterface, Get), stub_propertyGet);

    // Act: 调用 canStop
    auto ret = m_tester->canStop();

    // Assert: 返回错误上下文与默认 false
    EXPECT_TRUE(ret.first.isValid());
    EXPECT_EQ(ret.first.getCode(), ErrorContext::kErrorTypeDBus);
    EXPECT_FALSE(ret.second);
}

TEST_F(UT_Systemd1UnitInterface, CanStop_ErrorMessageReply_ReturnsNoErrorAndFalse)
{
    // Arrange: 不注入属性 -> 桩返回 ErrorMessage 应答
    Stub stub;
    stub.set(ADDR(DBusPropertiesInterface, Get), stub_propertyGet);

    // Act: 调用 canStop
    auto ret = m_tester->canStop();

    // Assert: 无错误上下文，返回默认 false
    EXPECT_FALSE(ret.first.isValid());
    EXPECT_EQ(ret.first.getCode(), ErrorContext::kErrorTypeNoError);
    EXPECT_FALSE(ret.second);
}

TEST_F(UT_Systemd1UnitInterface, CanStop_WrongTypeReply_ReturnsFalse)
{
    // Arrange: 注入字符串 CanStop（类型不符）
    Stub stub;
    g_propertyValues.insert("CanStop", QStringLiteral("no"));
    stub.set(ADDR(DBusPropertiesInterface, Get), stub_propertyGet);

    // Act: 调用 canStop
    auto ret = m_tester->canStop();

    // Assert: 类型不符返回默认 false
    EXPECT_FALSE(ret.first.isValid());
    EXPECT_EQ(ret.first.getCode(), ErrorContext::kErrorTypeNoError);
    EXPECT_FALSE(ret.second);
}
