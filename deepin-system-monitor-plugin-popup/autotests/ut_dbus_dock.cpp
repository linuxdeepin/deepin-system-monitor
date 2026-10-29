// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// DBusDockInterface
// (deepin-system-monitor-plugin-popup/dbus/dbusdockinterface.cpp/.h)单元测试
//
// 外部依赖处理:
//  - common::systemInfo():直接调用(仅读系统版本常量,无外部 IO);
//  - 属性读取/接口有效性:由 ut_popup_stub 登记表 knob 控制;
//  - PropertiesChanged(a{sv})解组:本地构造的 QDBusArgument 处于写模式,
//    qdbus_cast 无法解出内容 —— 因此通过 mock 服务的真实 DBus 信号发送,
//    测试进程接收到的消息为 read 模式 QDBusArgument,再转发给被测槽。

#include <gtest/gtest.h>

#include <QtTest>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QMetaObject>
#include <QRect>
#include <QSignalSpy>
#include <QVariant>
#include <QVariantMap>

#include "dbusdockinterface.h"
#include "helper.hpp"
#include "ut_popup_stub.h"

// PropertiesChanged 接收器:Qt6 QDBusConnection 仅有老式 receiver/SLOT 重载,
// 用 QObject 辅助类持有收到的消息(read 模式 a{sv})
class DockPropsChangedReceiver : public QObject
{
    Q_OBJECT
public slots:
    void onPropertiesChanged(const QDBusMessage &msg)
    {
        m_lastMessage = msg;
    }

public:
    QDBusMessage m_lastMessage;
};

class UT_DBusDock : public ::testing::Test
{
protected:
    void SetUp() override
    {
        ut_popup_stub::resetKnobs();
        m_receiver.m_lastMessage = QDBusMessage();
        // 空匹配规则:接收 mock 服务发来的任意路径 PropertiesChanged
        QDBusConnection::sessionBus().connect(QString(), QString(),
                                              QStringLiteral("org.freedesktop.DBus.Properties"),
                                              QStringLiteral("PropertiesChanged"),
                                              &m_receiver,
                                              SLOT(onPropertiesChanged(QDBusMessage)));
    }
    void TearDown() override
    {
        QDBusConnection::sessionBus().disconnect(QString(), QString(),
                                                 QStringLiteral("org.freedesktop.DBus.Properties"),
                                                 QStringLiteral("PropertiesChanged"),
                                                 &m_receiver,
                                                 SLOT(onPropertiesChanged(QDBusMessage)));
        ut_popup_stub::resetKnobs();
    }

protected:
    // 触发 mock 服务发送 PropertiesChanged(阻塞 knob 调用,信号异步送达)
    void triggerDockPropertiesChanged(const QString &ifaceName, const QStringList &props)
    {
        QDBusInterface config(QStringLiteral("org.deepin.dde.MockConfig"),
                              QStringLiteral("/org/deepin/dde/MockConfig"),
                              QStringLiteral("org.deepin.dde.MockConfig"),
                              QDBusConnection::sessionBus());
        config.call(QStringLiteral("sendDockPropertiesChanged"), ifaceName, props);
    }

    QDBusMessage lastMessage() const { return m_receiver.m_lastMessage; }

    DockPropsChangedReceiver m_receiver;
};

// 用例名称: staticInterfaceName_matchesSystemState
// 测试类型: 正常场景
// 覆盖函数: DBusDockInterface::staticInterfaceName(新旧版本分支)
// 操作步骤: 与 common::systemInfo() 的版本状态比对
// 预期结果: 旧版本返回 com.deepin.dde.Dock,否则返回 org.deepin.dde.Dock1
TEST_F(UT_DBusDock, staticInterfaceName_matchesSystemState)
{
    // Act
    const char *name = DBusDockInterface::staticInterfaceName();
    // Assert
    const bool isOld = common::systemInfo().isOldVersion();
    EXPECT_STREQ(name, isOld ? "com.deepin.dde.Dock" : "org.deepin.dde.Dock1");
}

// 用例名称: construction_servicePathInterfaceFromSystemState
// 测试类型: 正常场景
// 覆盖函数: DBusDockInterface::DBusDockInterface(PropertiesChanged 信号连接)
// 操作步骤: 构造接口,校验 service/path/interface 与系统状态一致
// 预期结果: 三要素均取自 common::systemInfo()
TEST_F(UT_DBusDock, construction_servicePathInterfaceFromSystemState)
{
    // Act
    DBusDockInterface dock;
    // Assert
    EXPECT_EQ(dock.service(), common::systemInfo().DockService);
    EXPECT_EQ(dock.path(), common::systemInfo().DockPath);
    EXPECT_EQ(dock.interface(), QString::fromLatin1(DBusDockInterface::staticInterfaceName()));
}

// 用例名称: isValid_knobControlled
// 测试类型: 边界场景
// 覆盖函数: DBusDockInterface 构造后接口有效性(桩登记表优先)
// 操作步骤: 未登记时查询,登记后再查询
// 预期结果: 未登记为 false,登记后为 true
TEST_F(UT_DBusDock, isValid_knobControlled)
{
    // Act & Assert: 未登记(桩兜底 false)
    DBusDockInterface dock;
    EXPECT_FALSE(dock.isValid());
    // Act & Assert: 登记 valid
    ut_popup_stub::setInterfaceValid(&dock, true);
    EXPECT_TRUE(dock.isValid());
}

// 用例名称: geometry_unregistered_returnsEmptyRect
// 测试类型: 边界场景
// 覆盖函数: DBusDockInterface::geometry(属性读取失败回落空矩形分支)
// 操作步骤: 不登记属性,调用 geometry()
// 预期结果: 返回默认 QRect()
TEST_F(UT_DBusDock, geometry_unregistered_returnsEmptyRect)
{
    // Act
    DBusDockInterface dock;
    // Assert
    EXPECT_EQ(dock.geometry(), QRect());
}

// 用例名称: geometry_registered_returnsInjectedRect
// 测试类型: 正常场景
// 覆盖函数: DBusDockInterface::geometry(qvariant_cast 转换分支)
// 操作步骤: 登记注入矩形 (1,2,3,4) 后调用 geometry()
// 预期结果: 返回注入的 QRect(1,2,3,4)
TEST_F(UT_DBusDock, geometry_registered_returnsInjectedRect)
{
    // Arrange
    DBusDockInterface dock;
    ut_popup_stub::setInterfaceProperty(&dock, QStringLiteral("geometry"), QRect(1, 2, 3, 4));
    // Act & Assert
    EXPECT_EQ(dock.geometry(), QRect(1, 2, 3, 4));
}

// 用例名称: propertyChanged_wrongArgCount_ignored
// 测试类型: 边界场景
// 覆盖函数: __propertyChanged__(参数个数非 3 提前返回分支)
// 操作步骤: 构造 2 参数消息,invokeMethod 调用私有槽
// 预期结果: 不发出 geometryChanged,无崩溃
TEST_F(UT_DBusDock, propertyChanged_wrongArgCount_ignored)
{
    // Arrange
    DBusDockInterface dock;
    QSignalSpy spy(&dock, &DBusDockInterface::geometryChanged);
    QDBusMessage msg = QDBusMessage::createSignal(QStringLiteral("/org/deepin/dde/Dock1"),
                                                  QStringLiteral("org.freedesktop.DBus.Properties"),
                                                  QStringLiteral("PropertiesChanged"));
    msg << QStringLiteral("org.deepin.dde.Dock1") << QVariantMap();
    // Act
    QMetaObject::invokeMethod(&dock, "__propertyChanged__", Q_ARG(QDBusMessage, msg));
    // Assert
    EXPECT_EQ(spy.count(), 0);
}

// 用例名称: propertyChanged_interfaceMismatch_ignored
// 测试类型: 边界场景
// 覆盖函数: __propertyChanged__(接口名不匹配提前返回分支)
// 操作步骤: 构造 3 参数消息,接口名与 DockService 不一致
// 预期结果: 不发出 geometryChanged,无崩溃
TEST_F(UT_DBusDock, propertyChanged_interfaceMismatch_ignored)
{
    // Arrange
    DBusDockInterface dock;
    QSignalSpy spy(&dock, &DBusDockInterface::geometryChanged);
    QDBusMessage msg = QDBusMessage::createSignal(QStringLiteral("/org/deepin/dde/Dock1"),
                                                  QStringLiteral("org.freedesktop.DBus.Properties"),
                                                  QStringLiteral("PropertiesChanged"));
    msg << QStringLiteral("org.other.Interface") << QVariantMap() << QStringList();
    // Act
    QMetaObject::invokeMethod(&dock, "__propertyChanged__", Q_ARG(QDBusMessage, msg));
    // Assert
    EXPECT_EQ(spy.count(), 0);
}

// 用例名称: propertyChanged_geometryProp_currentBehaviorNoSignal
// 测试类型: 正常场景
// 覆盖函数: __propertyChanged__(接口匹配 + a{sv} 解组 + 属性查找 + 通知信号调用分支)
// 操作步骤: 经 mock 服务真实发送 geometry 属性变化信号,测试进程收到
//           read 模式消息后转发给被测槽
// 预期结果: 属性查找命中 geometry,但 Q_PROPERTY(geometry READ geometry)
//           未声明 NOTIFY,notifySignal() 无效 → geometryChanged 不会发出
//           (疑似源码缺陷:属性变化信号永久失效,已标红待复核)
TEST_F(UT_DBusDock, propertyChanged_geometryProp_currentBehaviorNoSignal)
{
    // Arrange
    DBusDockInterface dock;
    QSignalSpy spy(&dock, &DBusDockInterface::geometryChanged);
    // Act: 真实 DBus 信号(保证 a{sv} 为 read 模式)
    triggerDockPropertiesChanged(common::systemInfo().DockService, { QStringLiteral("geometry") });
    QTRY_COMPARE(lastMessage().arguments().count(), 3);
    QMetaObject::invokeMethod(&dock, "__propertyChanged__", Q_ARG(QDBusMessage, lastMessage()));
    // Assert: 当前实现下信号不发出(缺陷感知)
    EXPECT_EQ(spy.count(), 0);
}

// 用例名称: propertyChanged_unmatchedProp_noCrash
// 测试类型: 边界场景
// 覆盖函数: __propertyChanged__(属性名不在元对象中,循环不命中分支)
// 操作步骤: 经 mock 服务发送 NonExistent 属性变化信号并转发
// 预期结果: 无崩溃,不发出 geometryChanged
TEST_F(UT_DBusDock, propertyChanged_unmatchedProp_noCrash)
{
    // Arrange
    DBusDockInterface dock;
    QSignalSpy spy(&dock, &DBusDockInterface::geometryChanged);
    // Act
    triggerDockPropertiesChanged(common::systemInfo().DockService, { QStringLiteral("NonExistent") });
    QTRY_COMPARE(lastMessage().arguments().count(), 3);
    QMetaObject::invokeMethod(&dock, "__propertyChanged__", Q_ARG(QDBusMessage, lastMessage()));
    // Assert
    EXPECT_EQ(spy.count(), 0);
}

// 用例名称: destructor_noCrash
// 测试类型: 边界场景
// 覆盖函数: DBusDockInterface::~DBusDockInterface
// 操作步骤: 作用域内构造并析构
// 预期结果: 析构无崩溃
TEST_F(UT_DBusDock, destructor_noCrash)
{
    {
        DBusDockInterface dock;
        Q_UNUSED(dock)
    }
    SUCCEED();
}

#include "ut_dbus_dock.moc"
