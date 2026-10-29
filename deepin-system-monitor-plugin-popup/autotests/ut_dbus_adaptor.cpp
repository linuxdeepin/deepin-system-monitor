// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// SystemMonitorDBusAdaptor
// (deepin-system-monitor-plugin-popup/dbus/systemmonitordbusadaptor.cpp)单元测试
// 纯 QObject + QDBusContext 适配器,无外部依赖,直接验证元对象契约与信号转发。

#include <gtest/gtest.h>

#include <QMetaObject>
#include <QSignalSpy>
#include <QString>

#include "systemmonitordbusadaptor.h"

class UT_DBusAdaptor : public ::testing::Test
{
protected:
    void SetUp() override
    {
    }
    void TearDown() override
    {
    }
};

// 用例名称: construction_metaObjectContract
// 测试类型: 正常场景
// 覆盖函数: SystemMonitorDBusAdaptor::SystemMonitorDBusAdaptor
// 操作步骤: 构造适配器,检查元对象中信号/槽/Q_CLASSINFO 契约
// 预期结果: 两个 Q_SCRIPTABLE 信号与一个 Q_SCRIPTABLE 槽均存在,
//           D-Bus Interface 为 com.deepin.SystemMonitorPluginPopup
TEST_F(UT_DBusAdaptor, construction_metaObjectContract)
{
    // Act
    SystemMonitorDBusAdaptor adaptor;
    const QMetaObject *mo = adaptor.metaObject();
    // Assert: 信号与槽齐全
    EXPECT_GE(mo->indexOfSignal("sigSendShowOrHideSystemMonitorPluginPopupWidget()"), 0);
    EXPECT_GE(mo->indexOfSignal("sysMonPopVisibleChanged(bool)"), 0);
    EXPECT_GE(mo->indexOfSlot("slotShowOrHideSystemMonitorPluginPopupWidget()"), 0);
    // Assert: D-Bus 接口名
    const int ci = mo->indexOfClassInfo("D-Bus Interface");
    ASSERT_GE(ci, 0);
    EXPECT_EQ(QString::fromLatin1(mo->classInfo(ci).value()),
              QStringLiteral("com.deepin.SystemMonitorPluginPopup"));
}

// 用例名称: slotShowOrHide_emitsSignal
// 测试类型: 正常场景
// 覆盖函数: SystemMonitorDBusAdaptor::slotShowOrHideSystemMonitorPluginPopupWidget
// 操作步骤: invokeMethod 触发 Q_SCRIPTABLE 槽,以 QSignalSpy 观测信号
// 预期结果: 发出一次 sigSendShowOrHideSystemMonitorPluginPopupWidget,无参数
TEST_F(UT_DBusAdaptor, slotShowOrHide_emitsSignal)
{
    // Arrange
    SystemMonitorDBusAdaptor adaptor;
    QSignalSpy spy(&adaptor, &SystemMonitorDBusAdaptor::sigSendShowOrHideSystemMonitorPluginPopupWidget);
    ASSERT_EQ(spy.count(), 0);
    // Act
    QMetaObject::invokeMethod(&adaptor, "slotShowOrHideSystemMonitorPluginPopupWidget");
    // Assert
    EXPECT_EQ(spy.count(), 1);
    ASSERT_EQ(spy.first().count(), 0);
}

// 用例名称: slotShowOrHide_repeatedEmitsEachTime
// 测试类型: 边界场景
// 覆盖函数: SystemMonitorDBusAdaptor::slotShowOrHideSystemMonitorPluginPopupWidget(重复调用)
// 操作步骤: 连续触发两次槽调用
// 预期结果: 每次调用各发出一次信号
TEST_F(UT_DBusAdaptor, slotShowOrHide_repeatedEmitsEachTime)
{
    // Arrange
    SystemMonitorDBusAdaptor adaptor;
    QSignalSpy spy(&adaptor, &SystemMonitorDBusAdaptor::sigSendShowOrHideSystemMonitorPluginPopupWidget);
    // Act
    QMetaObject::invokeMethod(&adaptor, "slotShowOrHideSystemMonitorPluginPopupWidget");
    QMetaObject::invokeMethod(&adaptor, "slotShowOrHideSystemMonitorPluginPopupWidget");
    // Assert
    EXPECT_EQ(spy.count(), 2);
}

// 用例名称: destructor_noCrash
// 测试类型: 边界场景
// 覆盖函数: SystemMonitorDBusAdaptor::~SystemMonitorDBusAdaptor
// 操作步骤: 作用域内构造并析构
// 预期结果: 析构无崩溃
TEST_F(UT_DBusAdaptor, destructor_noCrash)
{
    {
        SystemMonitorDBusAdaptor adaptor;
        Q_UNUSED(adaptor)
    }
    SUCCEED();
}
