// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Application / MonitorStartEvent / NetifStartEvent
// (deepin-system-monitor-plugin-popup/application.cpp)单元测试
//
// 说明:main-ut.cpp 已构造全局 Application 实例(main() 中 Application 构造
// 已覆盖元类型注册与监控线程挂载);QThread::start 已由 ut_popup_stub 进程级
// 打桩(BaseThread 派生线程不真正运行),故 event() 触发的 thread->start() 安全。
// 禁止在本文件再次构造 Application(第二个 QApplication 实例为未定义行为)。

#include <gtest/gtest.h>

#include <QEvent>
#include <QMetaType>

#include "application.h"
#include "ut_popup_stub.h"

class UT_Application : public ::testing::Test
{
protected:
    void SetUp() override
    {
        ut_popup_stub::resetKnobs();
    }
    void TearDown() override
    {
        ut_popup_stub::resetKnobs();
    }
};

// 用例名称: metatypes_registered
// 测试类型: 正常场景
// 覆盖函数: Application::Application(qRegisterMetaType 分支)
// 操作步骤: 复用 main-ut.cpp 构造的 Application 实例,查询已注册元类型
// 预期结果: TaskState/pid_t/ErrorContext 元类型均已注册可查
TEST_F(UT_Application, metatypes_registered)
{
    // Assert: main() 场景下 Application 构造注册的三类元类型均有效
    EXPECT_TRUE(QMetaType::fromName(QByteArray("Application::TaskState")).isValid());
    EXPECT_TRUE(QMetaType::fromName(QByteArray("pid_t")).isValid());
    EXPECT_TRUE(QMetaType::fromName(QByteArray("ErrorContext")).isValid());
}

// 用例名称: gApp_instanceIsValid
// 测试类型: 正常场景
// 覆盖函数: gApp 宏(qobject_cast<Application*>(Application::instance()))
// 操作步骤: 读取 gApp 并与 instance() 的转换结果比对
// 预期结果: gApp 非空且与 instance() 一致
TEST_F(UT_Application, gApp_instanceIsValid)
{
    // Assert
    Application *app = gApp;
    ASSERT_NE(app, nullptr);
    EXPECT_EQ(app, qobject_cast<Application *>(Application::instance()));
}

// 用例名称: event_customEvent_startsMonitorThread
// 测试类型: 正常场景
// 覆盖函数: Application::event(线程启动 + 基类转发分支)
// 操作步骤: 向 gApp 派发自定义类型事件,经桩计数观测 thread->start()
// 预期结果: 事件被处理返回 true,监控线程 start 被调用一次(桩拦截,不真跑)
TEST_F(UT_Application, event_customEvent_startsMonitorThread)
{
    // Arrange
    ASSERT_NE(gApp, nullptr);
    QEvent event(QEvent::User);
    const int before = ut_popup_stub::g_threadStartCount;
    // Act: 任意事件都会触发监控线程启动(实现不区分事件类型)
    const bool handled = gApp->event(&event);
    // Assert
    EXPECT_TRUE(handled);
    EXPECT_EQ(ut_popup_stub::g_threadStartCount, before + 1);
}

// 用例名称: event_paintEvent_startsMonitorThread
// 测试类型: 边界场景
// 覆盖函数: Application::event(普通界面事件同样触发线程启动,感知实现缺陷)
// 操作步骤: 向 gApp 派发 Paint 事件
// 预期结果: 返回 true 且线程启动计数增加(实现未过滤事件类型,存在每次
//           Paint 都重启线程的嫌疑,已标红待复核)
TEST_F(UT_Application, event_paintEvent_startsMonitorThread)
{
    // Arrange
    QEvent paint(QEvent::Paint);
    const int before = ut_popup_stub::g_threadStartCount;
    // Act: 实现不区分事件类型,普通界面事件同样触发线程启动
    gApp->event(&paint);
    // Assert: 仅断言线程启动副作用(基类对 Paint 事件的返回值随
    //         QApplication 版本实现而定,不属于被测代码的行为)
    EXPECT_EQ(ut_popup_stub::g_threadStartCount, before + 1);
}

// 用例名称: monitorStartEvent_typeMatchesConstant
// 测试类型: 正常场景
// 覆盖函数: MonitorStartEvent 构造/析构(QEvent::User+1 类型事件)
// 操作步骤: 构造 MonitorStartEvent,校验事件类型,作用域结束析构
// 预期结果: type() 等于 kMonitorStartEventType
TEST_F(UT_Application, monitorStartEvent_typeMatchesConstant)
{
    // Act
    MonitorStartEvent event;
    // Assert
    EXPECT_EQ(event.type(), kMonitorStartEventType);
    EXPECT_EQ(kMonitorStartEventType, static_cast<QEvent::Type>(QEvent::User + 1));
}

// 用例名称: netifStartEvent_typeMatchesConstant
// 测试类型: 正常场景
// 覆盖函数: NetifStartEvent 构造/析构(QEvent::User+2 类型事件)
// 操作步骤: 构造 NetifStartEvent,校验事件类型,作用域结束析构
// 预期结果: type() 等于 kNetifStartEventType
TEST_F(UT_Application, netifStartEvent_typeMatchesConstant)
{
    // Act
    NetifStartEvent event;
    // Assert
    EXPECT_EQ(event.type(), kNetifStartEventType);
    EXPECT_EQ(kNetifStartEventType, static_cast<QEvent::Type>(QEvent::User + 2));
}
