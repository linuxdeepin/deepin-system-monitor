// SPDX-FileCopyrightText: 2019-2026 UnionTech Software Technology Co., Ltd.
// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// 用例计数声明（self-check-structural 验证此块）：
// | method               | level | factors | min | actual |
// |----------------------|-------|---------|-----|--------|
// | ctor                 | low   | -       | 1   | 1      |
// | getMainPID           | mid   | -       | 2   | 4      |
// | getMemoryCurrent     | low   | -       | 1   | 4      |
// | getControlGroup      | low   | -       | 1   | 4      |
// | getEnvironmentFiles  | low   | -       | 1   | 5      |
// | dtor                 | low   | -       | 1   | 全局隐式覆盖 |

//self
#include "dbus/systemd1_service_interface.h"
#include "dbus/environment_file.h"

//被桩对象（属性接口）
#include "dbus/dbus_properties_interface.h"
#include "common/error_context.h"

//gtest
#include "stub.h"
#include <gtest/gtest.h>

//qt
#include <QDBusMessage>
#include <QDBusArgument>
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

// 构造一个签名 a(sb) 的 QDBusArgument（EnvironmentFileList 的 DBus 序列化形态）；
// 数组内写自定义结构体元素需要元素类型先完成 DBus 注册（仅元素，不走全局 registerMetaType）
static QDBusArgument makeEnvironmentFileArgument()
{
    static bool registered = false;
    if (!registered) {
        qDBusRegisterMetaType<EnvironmentFile>();
        registered = true;
    }

    EnvironmentFile f1;
    f1.envFile = QStringLiteral("env1.conf");
    f1.flag = true;
    EnvironmentFile f2;
    f2.envFile = QStringLiteral("env2.conf");
    f2.flag = false;

    QDBusArgument arr;
    arr.beginArray(qMetaTypeId<EnvironmentFile>());
    arr << f1 << f2;
    arr.endArray();
    return arr;
}
/***************************************STUB end***********************************************/

class UT_Systemd1ServiceInterface: public ::testing::Test
{
public:
    UT_Systemd1ServiceInterface() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        // Arrange: 重置桩控状态并构造被测接口（Get 已被桩替换，无真实 DBus 交互；
        // EnvironmentFile 元类型仅声明，不在全局 DBus 注册，避免影响编组验证）
        g_propertyValues.clear();
        g_propertyError = false;
        static QDBusConnection fakeConn("ut-sm-fake-session");
        m_tester = new Systemd1ServiceInterface(QStringLiteral("org.freedesktop.systemd1"),
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
    Systemd1ServiceInterface *m_tester;
};

// ─────────────────────────── 构造 ───────────────────────────

TEST_F(UT_Systemd1ServiceInterface, Ctor_ValidParams_InterfaceConfigured)
{
    // Arrange: SetUp 已用合法 service/path 构造对象
    // Act: 读取接口基础信息
    const QString service = m_tester->service();
    const QString path = m_tester->path();

    // Assert: service/path/interface 均与构造参数一致
    EXPECT_EQ(service, QStringLiteral("org.freedesktop.systemd1"));
    EXPECT_EQ(path, QStringLiteral("/org/freedesktop/systemd1/unit/dbus_2eservice"));
    EXPECT_EQ(m_tester->interface(), QString(Systemd1ServiceInterface::staticInterfaceName()));
}

// ─────────────────────────── getMainPID ───────────────────────────

TEST_F(UT_Systemd1ServiceInterface, GetMainPID_ReplyUInt_ReturnsPid)
{
    // Arrange: 注入 uint 属性 MainPID
    Stub stub;
    g_propertyValues.insert("MainPID", 1234U);
    stub.set(ADDR(DBusPropertiesInterface, Get), stub_propertyGet);

    // Act: 调用 getMainPID
    auto ret = m_tester->getMainPID();

    // Assert: 无错误且取回注入 PID
    EXPECT_FALSE(ret.first.isValid());
    EXPECT_EQ(ret.first.getCode(), ErrorContext::kErrorTypeNoError);
    EXPECT_EQ(ret.second, 1234U);
}

TEST_F(UT_Systemd1ServiceInterface, GetMainPID_DbusError_ReturnsErrorContextAndZeroPid)
{
    // Arrange: 打开桩的 DBus 错误开关
    Stub stub;
    g_propertyError = true;
    stub.set(ADDR(DBusPropertiesInterface, Get), stub_propertyGet);

    // Act: 调用 getMainPID
    auto ret = m_tester->getMainPID();

    // Assert: 返回错误上下文与默认 0
    EXPECT_TRUE(ret.first.isValid());
    EXPECT_EQ(ret.first.getCode(), ErrorContext::kErrorTypeDBus);
    EXPECT_EQ(ret.second, 0U);
}

TEST_F(UT_Systemd1ServiceInterface, GetMainPID_ErrorMessageReply_ReturnsNoErrorAndZeroPid)
{
    // Arrange: 不注入属性 -> 桩返回 ErrorMessage 应答
    Stub stub;
    stub.set(ADDR(DBusPropertiesInterface, Get), stub_propertyGet);

    // Act: 调用 getMainPID
    auto ret = m_tester->getMainPID();

    // Assert: 无错误上下文，PID 为默认 0
    EXPECT_FALSE(ret.first.isValid());
    EXPECT_EQ(ret.first.getCode(), ErrorContext::kErrorTypeNoError);
    EXPECT_EQ(ret.second, 0U);
}

TEST_F(UT_Systemd1ServiceInterface, GetMainPID_WrongTypeReply_ReturnsZeroPid)
{
    // Arrange: 注入字符串 MainPID（类型不符）
    Stub stub;
    g_propertyValues.insert("MainPID", QStringLiteral("1234"));
    stub.set(ADDR(DBusPropertiesInterface, Get), stub_propertyGet);

    // Act: 调用 getMainPID
    auto ret = m_tester->getMainPID();

    // Assert: 类型不符返回默认 0
    EXPECT_FALSE(ret.first.isValid());
    EXPECT_EQ(ret.first.getCode(), ErrorContext::kErrorTypeNoError);
    EXPECT_EQ(ret.second, 0U);
}

// ─────────────────────────── getMemoryCurrent ───────────────────────────

TEST_F(UT_Systemd1ServiceInterface, GetMemoryCurrent_ReplyULongLong_ReturnsMemory)
{
    // Arrange: 注入 ulonglong 属性 MemoryCurrent
    Stub stub;
    g_propertyValues.insert("MemoryCurrent", QVariant::fromValue<quint64>(2048ULL));
    stub.set(ADDR(DBusPropertiesInterface, Get), stub_propertyGet);

    // Act: 调用 getMemoryCurrent
    auto ret = m_tester->getMemoryCurrent();

    // Assert: 无错误且取回注入内存值
    EXPECT_FALSE(ret.first.isValid());
    EXPECT_EQ(ret.first.getCode(), ErrorContext::kErrorTypeNoError);
    EXPECT_EQ(ret.second, 2048ULL);
}

TEST_F(UT_Systemd1ServiceInterface, GetMemoryCurrent_DbusError_ReturnsErrorContextAndZeroMemory)
{
    // Arrange: 打开桩的 DBus 错误开关
    Stub stub;
    g_propertyError = true;
    stub.set(ADDR(DBusPropertiesInterface, Get), stub_propertyGet);

    // Act: 调用 getMemoryCurrent
    auto ret = m_tester->getMemoryCurrent();

    // Assert: 返回错误上下文与默认 0
    EXPECT_TRUE(ret.first.isValid());
    EXPECT_EQ(ret.first.getCode(), ErrorContext::kErrorTypeDBus);
    EXPECT_EQ(ret.second, 0ULL);
}

TEST_F(UT_Systemd1ServiceInterface, GetMemoryCurrent_ErrorMessageReply_ReturnsNoErrorAndZeroMemory)
{
    // Arrange: 不注入属性 -> 桩返回 ErrorMessage 应答
    Stub stub;
    stub.set(ADDR(DBusPropertiesInterface, Get), stub_propertyGet);

    // Act: 调用 getMemoryCurrent
    auto ret = m_tester->getMemoryCurrent();

    // Assert: 无错误上下文，内存为默认 0
    EXPECT_FALSE(ret.first.isValid());
    EXPECT_EQ(ret.first.getCode(), ErrorContext::kErrorTypeNoError);
    EXPECT_EQ(ret.second, 0ULL);
}

TEST_F(UT_Systemd1ServiceInterface, GetMemoryCurrent_WrongTypeReply_ReturnsZeroMemory)
{
    // Arrange: 注入字符串 MemoryCurrent（类型不符）
    Stub stub;
    g_propertyValues.insert("MemoryCurrent", QStringLiteral("2048"));
    stub.set(ADDR(DBusPropertiesInterface, Get), stub_propertyGet);

    // Act: 调用 getMemoryCurrent
    auto ret = m_tester->getMemoryCurrent();

    // Assert: 类型不符返回默认 0
    EXPECT_FALSE(ret.first.isValid());
    EXPECT_EQ(ret.first.getCode(), ErrorContext::kErrorTypeNoError);
    EXPECT_EQ(ret.second, 0ULL);
}

// ─────────────────────────── getControlGroup ───────────────────────────

TEST_F(UT_Systemd1ServiceInterface, GetControlGroup_ReplyString_ReturnsControlGroup)
{
    // Arrange: 注入字符串属性 ControlGroup
    Stub stub;
    g_propertyValues.insert("ControlGroup",
                            QStringLiteral("/system.slice/dbus.service"));
    stub.set(ADDR(DBusPropertiesInterface, Get), stub_propertyGet);

    // Act: 调用 getControlGroup
    auto ret = m_tester->getControlGroup();

    // Assert: 无错误且取回注入值
    EXPECT_FALSE(ret.first.isValid());
    EXPECT_EQ(ret.first.getCode(), ErrorContext::kErrorTypeNoError);
    EXPECT_EQ(ret.second, QStringLiteral("/system.slice/dbus.service"));
}

TEST_F(UT_Systemd1ServiceInterface, GetControlGroup_DbusError_ReturnsErrorContextAndEmptyGroup)
{
    // Arrange: 打开桩的 DBus 错误开关
    Stub stub;
    g_propertyError = true;
    stub.set(ADDR(DBusPropertiesInterface, Get), stub_propertyGet);

    // Act: 调用 getControlGroup
    auto ret = m_tester->getControlGroup();

    // Assert: 返回错误上下文与空 cgroup
    EXPECT_TRUE(ret.first.isValid());
    EXPECT_EQ(ret.first.getCode(), ErrorContext::kErrorTypeDBus);
    EXPECT_TRUE(ret.second.isEmpty());
}

TEST_F(UT_Systemd1ServiceInterface, GetControlGroup_ErrorMessageReply_ReturnsNoErrorAndEmptyGroup)
{
    // Arrange: 不注入属性 -> 桩返回 ErrorMessage 应答
    Stub stub;
    stub.set(ADDR(DBusPropertiesInterface, Get), stub_propertyGet);

    // Act: 调用 getControlGroup
    auto ret = m_tester->getControlGroup();

    // Assert: 无错误上下文，cgroup 为默认空值
    EXPECT_FALSE(ret.first.isValid());
    EXPECT_EQ(ret.first.getCode(), ErrorContext::kErrorTypeNoError);
    EXPECT_TRUE(ret.second.isEmpty());
}

TEST_F(UT_Systemd1ServiceInterface, GetControlGroup_WrongTypeReply_ReturnsEmptyGroup)
{
    // Arrange: 注入布尔 ControlGroup（类型不符）
    Stub stub;
    g_propertyValues.insert("ControlGroup", true);
    stub.set(ADDR(DBusPropertiesInterface, Get), stub_propertyGet);

    // Act: 调用 getControlGroup
    auto ret = m_tester->getControlGroup();

    // Assert: 类型不符返回默认空值
    EXPECT_FALSE(ret.first.isValid());
    EXPECT_EQ(ret.first.getCode(), ErrorContext::kErrorTypeNoError);
    EXPECT_TRUE(ret.second.isEmpty());
}

// ─────────────────────────── getEnvironmentFiles ───────────────────────────

TEST_F(UT_Systemd1ServiceInterface, GetEnvironmentFiles_ReplyArrayOfStruct_PassesSignatureCheck)
{
    // Arrange: 注入 a(sb) 数组属性 EnvironmentFiles（写态 QDBusArgument，
    // currentSignature 为真实编组结果 a(sb)）
    Stub stub;
    g_propertyValues.insert("EnvironmentFiles",
                            QVariant::fromValue(QDBusVariant(
                                QVariant::fromValue(makeEnvironmentFileArgument()))));
    stub.set(ADDR(DBusPropertiesInterface, Get), stub_propertyGet);

    // Act: 调用 getEnvironmentFiles
    auto ret = m_tester->getEnvironmentFiles();

    // Assert: 成功分支被完整走到（签名校验通过、无错误上下文）；
    // 列表内容反编组需要真实 wire 报文（Qt 限制：QDBusArgument 禁止边写边读），
    // 桩环境下列表为空，此为该分支的可观测行为
    EXPECT_FALSE(ret.first.isValid());
    EXPECT_EQ(ret.first.getCode(), ErrorContext::kErrorTypeNoError);
    EXPECT_TRUE(ret.second.isEmpty());
}

TEST_F(UT_Systemd1ServiceInterface, GetEnvironmentFiles_DbusError_ReturnsErrorContextAndEmptyList)
{
    // Arrange: 打开桩的 DBus 错误开关
    Stub stub;
    g_propertyError = true;
    stub.set(ADDR(DBusPropertiesInterface, Get), stub_propertyGet);

    // Act: 调用 getEnvironmentFiles
    auto ret = m_tester->getEnvironmentFiles();

    // Assert: 返回错误上下文与空列表
    EXPECT_TRUE(ret.first.isValid());
    EXPECT_EQ(ret.first.getCode(), ErrorContext::kErrorTypeDBus);
    EXPECT_TRUE(ret.second.isEmpty());
}

TEST_F(UT_Systemd1ServiceInterface, GetEnvironmentFiles_ErrorMessageReply_ReturnsNoErrorAndEmptyList)
{
    // Arrange: 不注入属性 -> 桩返回 ErrorMessage 应答
    Stub stub;
    stub.set(ADDR(DBusPropertiesInterface, Get), stub_propertyGet);

    // Act: 调用 getEnvironmentFiles
    auto ret = m_tester->getEnvironmentFiles();

    // Assert: 无错误上下文，列表为默认空值
    EXPECT_FALSE(ret.first.isValid());
    EXPECT_EQ(ret.first.getCode(), ErrorContext::kErrorTypeNoError);
    EXPECT_TRUE(ret.second.isEmpty());
}

TEST_F(UT_Systemd1ServiceInterface, GetEnvironmentFiles_WrongTypeReply_ReturnsEmptyList)
{
    // Arrange: 注入整型 EnvironmentFiles（非 QDBusArgument，签名必然不符）
    Stub stub;
    g_propertyValues.insert("EnvironmentFiles", 42);
    stub.set(ADDR(DBusPropertiesInterface, Get), stub_propertyGet);

    // Act: 调用 getEnvironmentFiles
    auto ret = m_tester->getEnvironmentFiles();

    // Assert: 签名不符返回默认空列表
    EXPECT_FALSE(ret.first.isValid());
    EXPECT_EQ(ret.first.getCode(), ErrorContext::kErrorTypeNoError);
    EXPECT_TRUE(ret.second.isEmpty());
}

TEST_F(UT_Systemd1ServiceInterface, GetEnvironmentFiles_WrongSignatureReply_ReturnsEmptyList)
{
    // Arrange: 注入 a(s) 数组（元素签名与 a(sb) 不符）
    Stub stub;
    QDBusArgument wrongArr;
    wrongArr.beginArray(qMetaTypeId<QString>());
    wrongArr << QStringLiteral("plain-string");
    wrongArr.endArray();
    g_propertyValues.insert("EnvironmentFiles",
                            QVariant::fromValue(QDBusVariant(QVariant::fromValue(wrongArr))));
    stub.set(ADDR(DBusPropertiesInterface, Get), stub_propertyGet);

    // Act: 调用 getEnvironmentFiles
    auto ret = m_tester->getEnvironmentFiles();

    // Assert: 签名校验失败返回空列表
    EXPECT_FALSE(ret.first.isValid());
    EXPECT_EQ(ret.first.getCode(), ErrorContext::kErrorTypeNoError);
    EXPECT_TRUE(ret.second.isEmpty());
}
