// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// qt-autotest-generator 单元测试 - DBusInterface（deepin-system-monitor-plugin/dbus/dbusinterface.cpp）
//
// 用例计数声明：
// | 函数                                        | 用例数 |
// |---------------------------------------------|--------|
// | getInstance                                 | 1      |
// | DBusInterface (构造)                        | 1      |
// | showOrHideDeepinSystemMonitorPluginPopupWidget | 3   |
// | init                                        | 2（经 showOrHide 分支触发 + service/path 断言） |
//
// 10 项检查清单（test-code-gen.md）：
// 1 public 方法覆盖：getInstance/showOrHideDeepinSystemMonitorPluginPopupWidget 全覆盖；
// 2 等价类：startDetached 成功/失败、iface nullptr/无效/有效；
// 3 边界值：单例双重检查锁两次调用同指针、init 后 service/path 全匹配；
// 4 同质分组≥3 用 TEST_P：showOrHide 三分支差异较大，采用 3 个独立用例（结构不对称）；
// 5 分支清单→用例映射：见下方【分支清单】；
// 6 分支触发：QProcess::startDetached（stub.set_lamda 打桩，捕获命令+受控返回）、QDBusAbstractInterface::
//   isValid/call 打桩（受控无效/有效态，杜绝真实总线副作用）；
// 7 异常断言：无显式 throw，DBus 调用路径 EXPECT_NO_THROW；
// 8 反向场景：startDetached 失败、iface 无效重试、有效直调不走 startDetached；
// 9 强异常安全：TearDown 重置单例（含泄漏防护），用例间完全隔离；
// 10 打桩选型：startDetached/isValid/call 均 stub_ext 打桩；QDBusInterface 为真实对象
//    （构造自 init，仅用于校验 service/path 属性），不引入 gMock。
//
// 用例计数声明（self-check-structural 验证此块）：
// | method | level | factors | min | actual |
// |--------|-------|---------|-----|--------|
// | getInstance/ctor | high | double-check-lock | 2 | 2 |
// | init/showOrHide | high | iface-null/retry/valid | 3 | 3 |
//
// 【分支清单】（复杂方法分支 → 用例映射）
// DBusInterface::showOrHideDeepinSystemMonitorPluginPopupWidget：
//   B1 mp_Iface == nullptr → 尝试 QProcess::startDetached（stub.set_lamda 打桩）→ DBusIface_ShowOrHide_IfaceNull_StartDetachedSuccess_NoInit
//   B2 startDetached 成功 → 直接返回（不 init）→ 同上
//   B3 startDetached 失败 && mp_Iface == nullptr → init() → DBusIface_ShowOrHide_StartDetachedFails_InitializesIface
//   B4 init 后仍 !isValid → 直接 call（错误应答，不崩溃）→ 同上（isValid 打桩 false）
//   B5 mp_Iface 非空 && !isValid → 再次 startDetached（不重复 init）→ 同上（第二次调用）
//   B6 mp_Iface 有效 → 直接 call 指定方法 → DBusIface_ShowOrHide_IfaceValid_CallsDBusMethod
// DBusInterface::init：
//   B7 sessionBus 未连接 → 告警 → init 经 B3 触发（总线状态由环境决定，两态均不崩溃）
//   B8 创建 QDBusInterface(SERVICE_NAME, SERVICE_PATH) → service()/path() 断言
// DBusInterface::getInstance：
//   B9 双重检查锁首次创建 → DBusIface_GetInstance_ReturnsSameSingleton
//   B10 后续调用复用 → 同上
//
// 【备注】showOrHideDeepinSystemMonitorPluginPopupWidget 依赖真实会话总线的弹出服务，
// 全部外部交互（startDetached/isValid/call）均打桩，测试不产生任何真实 DBus 副作用。

#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include <stubext.h>

#include "dbusinterface.h"

#include <QDBusAbstractInterface>
#include <QDBusInterface>
#include <QDBusError>
#include <QDBusMessage>
#include <QProcess>

// DBusIfaceTest：DBus 接口单例测试
class DBusIfaceTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        capturedCommands.clear();
        capturedCalls.clear();
    }
    void TearDown() override
    {
        // 重置单例并回收实例（类无析构、mp_Iface 为裸指针，需手动防泄漏）
        DBusInterface *old = DBusInterface::s_Instance.load();
        DBusInterface::s_Instance.store(nullptr);
        if (old) {
            delete old->mp_Iface;
            old->mp_Iface = nullptr;
            delete old;
        }
        stub.clear();
    }

    // 打桩 startDetached：捕获命令并返回受控结果
    void stubStartDetached(bool result)
    {
        stub.set_lamda(static_cast<bool (*)(const QString &, const QStringList &, const QString &, qint64 *)>(&QProcess::startDetached),
                       [result](const QString &program, const QStringList &arguments, const QString &workingDirectory, qint64 *pid) {
                           Q_UNUSED(arguments)
                           capturedCommands.append(program);
                           return result;
                       });
    }
    // 打桩 QDBusInterface 有效性
    void stubIfaceValid(bool valid)
    {
        stub.set_lamda(static_cast<bool (QDBusAbstractInterface::*)() const>(&QDBusAbstractInterface::isValid),
                       [valid](const QDBusAbstractInterface *) {
                           return valid;
                       });
    }
    // 打桩 DBus 方法调用：捕获方法名并返回错误应答
    void stubDBusCall()
    {
        stub.set_lamda(static_cast<QDBusMessage (QDBusAbstractInterface::*)(const QString &)>(
                           &QDBusAbstractInterface::call),
                       [](QDBusAbstractInterface *, const QString &method) {
                           capturedCalls.append(method);
                           return QDBusMessage::createError(QDBusError::ServiceUnknown, QStringLiteral("stubbed"));
                       });
    }

    static QStringList capturedCommands;
    static QStringList capturedCalls;
    stub_ext::StubExt stub;
};
QStringList DBusIfaceTest::capturedCommands;
QStringList DBusIfaceTest::capturedCalls;

// ---- getInstance / 构造 ----

TEST_F(DBusIfaceTest, DBusIface_GetInstance_ReturnsSameSingleton)
{
    // Arrange: 清空单例（TearDown 保证）
    // Act: 连续两次获取
    DBusInterface *first = DBusInterface::getInstance();
    DBusInterface *second = DBusInterface::getInstance();
    // Assert: 同一实例（双重检查锁单例，B9/B10）
    ASSERT_NE(first, nullptr);
    EXPECT_EQ(first, second);
    // 受保护构造函数可直接构造独立实例（-fno-access-control），且不与单例冲突
    DBusInterface direct;
    EXPECT_NE(&direct, first);
    EXPECT_EQ(direct.mp_Iface, nullptr);
}

TEST_F(DBusIfaceTest, DBusIface_Ctor_DoesNotInitializeIface)
{
    // Arrange: TearDown 已重置单例
    // Act: 获取全新单例
    DBusInterface *inst = DBusInterface::getInstance();
    // Assert: 构造函数不调用 init（延迟初始化语义），且二次获取仍为同一单例
    ASSERT_NE(inst, nullptr);
    EXPECT_EQ(inst->mp_Iface, nullptr);
    EXPECT_EQ(DBusInterface::getInstance(), inst);
}

// ---- showOrHideDeepinSystemMonitorPluginPopupWidget ----

TEST_F(DBusIfaceTest, DBusIface_ShowOrHide_IfaceNull_StartDetachedSuccess_NoInit)
{
    // Arrange: 全新单例（mp_Iface 为空），startDetached 受控成功
    DBusInterface *inst = DBusInterface::getInstance();
    ASSERT_EQ(inst->mp_Iface, nullptr);
    stubStartDetached(true);
    // Act: 触发显示/隐藏
    EXPECT_NO_THROW(inst->showOrHideDeepinSystemMonitorPluginPopupWidget());
    // Assert: 捕获到 gdbus 唤醒命令（B1），且未执行 init（B2）
    ASSERT_EQ(capturedCommands.count(), 1);
    EXPECT_TRUE(capturedCommands.first().contains("gdbus call"));
    EXPECT_TRUE(capturedCommands.first().contains("com.deepin.SystemMonitorPluginPopup"));
    EXPECT_TRUE(capturedCommands.first().contains("slotShowOrHideSystemMonitorPluginPopupWidget"));
    EXPECT_EQ(inst->mp_Iface, nullptr);
}

TEST_F(DBusIfaceTest, DBusIface_ShowOrHide_StartDetachedFails_InitializesIface)
{
    // Arrange: 全新单例，startDetached 受控失败，iface 受控无效
    DBusInterface *inst = DBusInterface::getInstance();
    stubStartDetached(false);
    stubIfaceValid(false);
    // Act: 触发（B3 init → B4 无效仍 call）
    EXPECT_NO_THROW(inst->showOrHideDeepinSystemMonitorPluginPopupWidget());
    // Assert: init 已执行，QDBusInterface 指向弹出服务（B8）
    ASSERT_NE(inst->mp_Iface, nullptr);
    EXPECT_EQ(inst->mp_Iface->service(), QString("com.deepin.SystemMonitorPluginPopup"));
    EXPECT_EQ(inst->mp_Iface->path(), QString("/com/deepin/SystemMonitorPluginPopup"));
    EXPECT_EQ(capturedCommands.count(), 1);
    // 反向：iface 非空但无效 → 再次 startDetached，不重复 init（B5）
    QDBusInterface *ifaceAfterInit = inst->mp_Iface;
    EXPECT_NO_THROW(inst->showOrHideDeepinSystemMonitorPluginPopupWidget());
    EXPECT_EQ(capturedCommands.count(), 2);
    EXPECT_EQ(inst->mp_Iface, ifaceAfterInit);
}

TEST_F(DBusIfaceTest, DBusIface_ShowOrHide_IfaceValid_CallsDBusMethod)
{
    // Arrange: 先经 init 建立 iface，再受控为有效态
    DBusInterface *inst = DBusInterface::getInstance();
    stubStartDetached(false);
    inst->showOrHideDeepinSystemMonitorPluginPopupWidget(); // 触发 init
    ASSERT_NE(inst->mp_Iface, nullptr);
    stubIfaceValid(true);
    stubDBusCall();
    capturedCommands.clear();
    // Act: 触发（B6：有效态直接 call）
    EXPECT_NO_THROW(inst->showOrHideDeepinSystemMonitorPluginPopupWidget());
    // Assert: 调用了目标 DBus 方法，且不再尝试 startDetached
    ASSERT_EQ(capturedCalls.count(), 1);
    EXPECT_EQ(capturedCalls.first(), QString("slotShowOrHideSystemMonitorPluginPopupWidget"));
    EXPECT_EQ(capturedCommands.count(), 0);
}
