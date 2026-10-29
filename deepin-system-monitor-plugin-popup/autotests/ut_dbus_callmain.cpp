// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// DbusCallMainInterface
// (deepin-system-monitor-plugin-popup/dbus/dbuscallmaininterface.cpp)单元测试
//
// 外部依赖:com.deepin.SystemMonitorMain 主进程服务 —— 由私有总线上的
// dsm-popup-mock-service 提供 MockSystemMonitorMain 实现,真实走 DBus
// 方法调用链(空接口名派发),通过 MockConfig knob 读取/清空调用记录。

#include <gtest/gtest.h>

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QStringList>

#include "dbuscallmaininterface.h"
#include "ut_popup_stub.h"

namespace {

QDBusInterface mockConfigInterface()
{
    return QDBusInterface(QStringLiteral("org.deepin.dde.MockConfig"),
                          QStringLiteral("/org/deepin/dde/MockConfig"),
                          QStringLiteral("org.deepin.dde.MockConfig"),
                          QDBusConnection::sessionBus());
}

QStringList mockJumpCalls()
{
    QDBusInterface config = mockConfigInterface();
    QDBusReply<QStringList> reply = config.call(QStringLiteral("getJumpCalls"));
    return reply.isValid() ? reply.value() : QStringList();
}

void clearMockJumpCalls()
{
    QDBusInterface config = mockConfigInterface();
    config.call(QStringLiteral("clearJumpCalls"));
}

}  // namespace

class UT_DbusCallMain : public ::testing::Test
{
protected:
    void SetUp() override
    {
        ut_popup_stub::resetKnobs();
        clearMockJumpCalls();
    }
    void TearDown() override
    {
        clearMockJumpCalls();
        ut_popup_stub::resetKnobs();
    }
};

// 用例名称: getinstance_sameInstance
// 测试类型: 正常场景
// 覆盖函数: DbusCallMainInterface::getInstance(原子单例双检锁)+ 构造 + init
// 操作步骤: 多次调用 getInstance 比对指针
// 预期结果: 每次返回同一实例,构造与 init(创建 QDBusInterface)无崩溃
TEST_F(UT_DbusCallMain, getinstance_sameInstance)
{
    // Act
    DbusCallMainInterface *first = DbusCallMainInterface::getInstance();
    DbusCallMainInterface *second = DbusCallMainInterface::getInstance();
    // Assert
    EXPECT_NE(first, nullptr);
    EXPECT_EQ(first, second);
}

// 用例名称: init_createsInterface_reachableOverBus
// 测试类型: 正常场景
// 覆盖函数: DbusCallMainInterface::init(创建 mp_Iface)
// 操作步骤: 单例初始化后通过 mock 服务验证 DBus 链路可达
//           (jumpWidget 走通即证明 mp_Iface 已创建)
// 预期结果: 调用记录为空且链路查询本身成功
TEST_F(UT_DbusCallMain, init_createsInterface_reachableOverBus)
{
    // Act: getInstance 已触发 init;jumpWidget 走通即证明接口可达
    DbusCallMainInterface::getInstance()->jumpWidget(QStringLiteral("MSG_PING"));
    // Assert: 收到 ping 调用记录,说明 init 创建的接口可用
    EXPECT_EQ(mockJumpCalls(), QStringList { QStringLiteral("slotJumpDetailWidget:MSG_PING") });
}

// 用例名称: jumpWidget_detailCode_callsDetailSlot
// 测试类型: 正常场景
// 覆盖函数: DbusCallMainInterface::jumpWidget(msgCode != MSG_PROCESS 分支)
// 操作步骤: 传入普通消息码,经真实 DBus 调用 mock 服务
// 预期结果: mock 收到 slotJumpDetailWidget 调用,携带原始消息码
TEST_F(UT_DbusCallMain, jumpWidget_detailCode_callsDetailSlot)
{
    // Arrange
    clearMockJumpCalls();
    // Act
    DbusCallMainInterface::getInstance()->jumpWidget(QStringLiteral("MSG_DETAIL_1"));
    // Assert
    EXPECT_EQ(mockJumpCalls(), QStringList { QStringLiteral("slotJumpDetailWidget:MSG_DETAIL_1") });
}

// 用例名称: jumpWidget_processCode_callsProcessSlot
// 测试类型: 正常场景
// 覆盖函数: DbusCallMainInterface::jumpWidget(msgCode == MSG_PROCESS 分支)
// 操作步骤: 传入 MSG_PROCESS 消息码,经真实 DBus 调用 mock 服务
// 预期结果: mock 收到 slotJumpProcessWidget 调用,携带原始消息码
TEST_F(UT_DbusCallMain, jumpWidget_processCode_callsProcessSlot)
{
    // Arrange
    clearMockJumpCalls();
    // Act
    DbusCallMainInterface::getInstance()->jumpWidget(QStringLiteral("MSG_PROCESS"));
    // Assert
    EXPECT_EQ(mockJumpCalls(), QStringList { QStringLiteral("slotJumpProcessWidget:MSG_PROCESS") });
}
