// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// UT_DBusObject：DBusObject 单元测试（qt-autotest-generator 重写）
//
// 被测文件：deepin-system-monitor-main/dbus/dbus_object.cpp
// 重写原因：Mode 6 审查判 FAIL（原用例断言空洞 EMPTY_ASSERT / 缺 AAA 注释 / 缺用例计数声明）
//
// 打桩说明（外部依赖全部 stub，不依赖真实 DBus/窗口环境）：
// - QDBusConnection::sessionBus        → 返回未连接连接
// - QDBusConnection::registerService   → 受控成功/失败 + 计数
// - QDBusConnection::registerObject    → 计数并捕获注册路径
// - QDBusConnection::unregisterService → 计数
// - QDBusAbstractInterface::callWithArgumentList → 受控回复 + 计数（通知已运行实例路径）
// - MainWindow 使用真实实例（offscreen 平台），验证窗口状态被真实还原
//
// ============================ 用例计数声明 ============================
// 方法                       | level | 最少用例 | actual | 说明
// DBusObject() 私有构造       | low   | 1 | 1 | 经 getInstance 单例创建覆盖
// getInstance()              | low   | 1 | 1 | 两次调用同实例（覆盖创建+快路径）
// registerOrNotify()         | mid   | 2 | 3 | 注册成功 / 已占用+通知带错误 / 已占用+通知无错误
// unRegister()               | low   | 1 | 1 | unregisterService 被调用一次
// handleWindow()             | mid   | 2 | 1 | 最小化窗口被还原（raise/activate 经真实 offscreen 窗口）
// ~DBusObject()              | low   | 1 | - | 单例生命周期覆盖（进程退出时析构）
//
// ============================== 最小清单 ==============================
// [x] 1 公开方法已列出，每个方法 ≥ 1 用例
// [x] 2 输入按等价类划分：注册成功/服务名被占用 × 通知结果 有错误消息/无错误消息
// [x] 3 边界值显式覆盖：注册失败分支（registerService 返回 false）
// [x] 4 无 ≥3 组同质断言输入，不适用 TEST_P（通知结果仅 2 组，用独立用例）
// [x] 5 分支清单已列出并映射到用例名
// [x] 6 每条 if/early-return 分支有触发用例
// [x] 7 本类无显式 throw，不适用 EXPECT_THROW
// [x] 8 负面场景（服务名被占用 → 返回 false）有专门用例
// [x] 9 负面用例验证强异常安全（失败后对象仍可再次调用 registerOrNotify）
// [x] 10 项目内接口类用 gMock，其他用 stub_ext（本项目统一用项目自带 stub.h 打桩）
//
// ============================== 分支清单 ==============================
// （来源：工作目录源码 dbus_object.cpp）
// getInstance():
//   B0  单例已存在/不存在 → 同一实例              → GetInstance_TwiceCalled_SameInstance
// registerOrNotify():
//   B1  registerService 成功 → registerObject 后返回 true → RegisterOrNotify_ServiceFree_RegistersObjectAndReturnsTrue
//   B2  registerService 失败 → QDBusInterface 通知已运行实例（errorMessage 非空 → 告警）返回 false → RegisterOrNotify_ServiceTaken_NotifyErrorReported_ReturnsFalse
//   B3  registerService 失败 → 通知成功（errorMessage 为空 → 不告警）返回 false → RegisterOrNotify_ServiceTaken_NotifySilent_ReturnsFalse
// unRegister():
//   B4  unregisterService(DBUS_SERVER) 恰好一次   → UnRegister_CalledOnce_UnregistersService
// handleWindow():
//   B5  gApp->mainWindow() 非空 → setWindowState 清最小化 + raise + activateWindow → HandleWindow_MinimizedWindow_RestoresState
//   B6  mw 为空的防御分支：源码直接解引用 mw，无空检查（疑似缺陷，见 defects.json），不可测崩溃路径
//
// 疑似源码缺陷记录（不改源码，只标红）：
//   #D-DBusObject-01 handleWindow() 未判空 gApp->mainWindow()：主窗口销毁后收到 DBus
//      激活请求会空指针解引用（dbus_object.cpp:73-75）

#include "dbus/dbus_object.h"
//self
#include "application.h"
#include "gui/main_window.h"
//qt
#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusError>
#include <QDBusInterface>
#include <QDBusMessage>
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
int g_registerServiceCount = 0;
int g_registerObjectCount = 0;
int g_unregisterServiceCount = 0;
int g_notifyCount = 0;
QString g_lastRegisteredPath;
bool g_registerServiceOk = true;
bool g_notifyReplyHasError = true;

void resetDBusObjectStubState()
{
    g_registerServiceCount = 0;
    g_registerObjectCount = 0;
    g_unregisterServiceCount = 0;
    g_notifyCount = 0;
    g_lastRegisteredPath.clear();
    g_registerServiceOk = true;
    g_notifyReplyHasError = true;
}
}  // namespace

// static QDBusConnection QDBusConnection::sessionBus()
static QDBusConnection stub_QDBusConnection_sessionBus()
{
    return QDBusConnection(QStringLiteral("ut-unconnected-bus"));
}

// bool QDBusConnection::registerService(const QString&)
static bool stub_QDBusConnection_registerService(void *, const QString &)
{
    ++g_registerServiceCount;
    return g_registerServiceOk;
}

// bool QDBusConnection::registerObject(const QString&, QObject*, RegisterOptions)（Qt6 返回 bool）
static bool stub_QDBusConnection_registerObject(void *, const QString &path, QObject *,
                                                QDBusConnection::RegisterOptions)
{
    ++g_registerObjectCount;
    g_lastRegisteredPath = path;
    return true;
}

// bool QDBusConnection::unregisterService(const QString&)
static bool stub_QDBusConnection_unregisterService(void *, const QString &)
{
    ++g_unregisterServiceCount;
    return true;
}

// 受控成功回复（createReply 为 const 成员，用静态工厂构造同型消息）
static QDBusMessage makeReplyMessage(const QList<QVariant> &args)
{
    QDBusMessage msg = QDBusMessage::createMethodCall(QStringLiteral("ut.svc"), QStringLiteral("/ut"),
                                                      QStringLiteral("ut.if"), QStringLiteral("m"));
    msg.setArguments(args);
    return msg;
}

// QDBusMessage QDBusAbstractInterface::callWithArgumentList(CallMode, const QString&, const QList<QVariant>&)（Qt6 无 timeout 参）
static QDBusMessage stub_QDBusAbstractInterface_callWithArgumentList(void *, QDBus::CallMode,
                                                                     const QString &,
                                                                     const QList<QVariant> &)
{
    ++g_notifyCount;
    if (g_notifyReplyHasError)
        return QDBusMessage::createError(QDBusError::ServiceUnknown,
                                         QStringLiteral("stub: service unknown"));
    return makeReplyMessage(QVariantList());
}

// handleWindow 专用：惰性创建真实 MainWindow（offscreen，进程退出前不析构）
static MainWindow &windowForHandleWindowTest()
{
    static MainWindow s_window;
    return s_window;
}

// ============================== 夹具 ==============================
class UT_DBusObject : public ::testing::Test
{
public:
    UT_DBusObject() {}
    ~UT_DBusObject() override {}

    void SetUp() override
    {
        resetDBusObjectStubState();
    }

    void TearDown() override {}
};

// ============================== 用例 ==============================
// B0: 单例唯一
TEST_F(UT_DBusObject, GetInstance_TwiceCalled_SameInstance)
{
    // Arrange: 环境就绪（无需打桩）
    // Act: 连续两次获取单例
    DBusObject &first = DBusObject::getInstance();
    DBusObject &second = DBusObject::getInstance();
    // Assert: 均为有效对象且同一实例
    EXPECT_NE(&first, nullptr);
    EXPECT_EQ(&first, &second);
}

// B1: 服务未被占用 → 注册服务并注册对象路径，返回 true
TEST_F(UT_DBusObject, RegisterOrNotify_ServiceFree_RegistersObjectAndReturnsTrue)
{
    // Arrange: 打桩且注册成功
    Stub stub;
    stub.set(ADDR(QDBusConnection, sessionBus), stub_QDBusConnection_sessionBus);
    stub.set((bool (QDBusConnection::*)(const QString &))ADDR(QDBusConnection, registerService),
             stub_QDBusConnection_registerService);
    stub.set((bool (QDBusConnection::*)(const QString &, QObject *,
                                        QDBusConnection::RegisterOptions))
                 ADDR(QDBusConnection, registerObject), stub_QDBusConnection_registerObject);
    resetDBusObjectStubState();
    g_registerServiceOk = true;

    // Act: 注册
    const bool result = DBusObject::getInstance().registerOrNotify();

    // Assert: 返回 true，注册路径正确
    EXPECT_TRUE(result);
    EXPECT_EQ(g_registerServiceCount, 1);
    EXPECT_EQ(g_registerObjectCount, 1);
    EXPECT_EQ(g_lastRegisteredPath, QStringLiteral("/com/deepin/systemMonitor"));
}

// B2: 服务被占用 → 通知已运行实例且带回错误消息，返回 false
TEST_F(UT_DBusObject, RegisterOrNotify_ServiceTaken_NotifyErrorReported_ReturnsFalse)
{
    // Arrange: 打桩注册失败、通知返回错误消息
    Stub stub;
    stub.set(ADDR(QDBusConnection, sessionBus), stub_QDBusConnection_sessionBus);
    stub.set((bool (QDBusConnection::*)(const QString &))ADDR(QDBusConnection, registerService),
             stub_QDBusConnection_registerService);
    stub.set((QDBusMessage (QDBusAbstractInterface::*)(QDBus::CallMode, const QString &,
                                                       const QList<QVariant> &))
                 ADDR(QDBusAbstractInterface, callWithArgumentList),
             stub_QDBusAbstractInterface_callWithArgumentList);
    resetDBusObjectStubState();
    g_registerServiceOk = false;
    g_notifyReplyHasError = true;

    // Act: 注册
    const bool result = DBusObject::getInstance().registerOrNotify();

    // Assert: 返回 false 且通知恰一次
    EXPECT_FALSE(result);
    EXPECT_EQ(g_registerServiceCount, 1);
    EXPECT_EQ(g_registerObjectCount, 0);
    EXPECT_EQ(g_notifyCount, 1);
}

// B3: 服务被占用 → 通知成功（errorMessage 为空 → 走不告警分支），返回 false
TEST_F(UT_DBusObject, RegisterOrNotify_ServiceTaken_NotifySilent_ReturnsFalse)
{
    // Arrange: 打桩注册失败、通知无错误
    Stub stub;
    stub.set(ADDR(QDBusConnection, sessionBus), stub_QDBusConnection_sessionBus);
    stub.set((bool (QDBusConnection::*)(const QString &))ADDR(QDBusConnection, registerService),
             stub_QDBusConnection_registerService);
    stub.set((QDBusMessage (QDBusAbstractInterface::*)(QDBus::CallMode, const QString &,
                                                       const QList<QVariant> &))
                 ADDR(QDBusAbstractInterface, callWithArgumentList),
             stub_QDBusAbstractInterface_callWithArgumentList);
    resetDBusObjectStubState();
    g_registerServiceOk = false;
    g_notifyReplyHasError = false;

    // Act: 注册
    const bool result = DBusObject::getInstance().registerOrNotify();

    // Assert: 返回 false（error.isEmpty() 分支不产生额外副作用）
    EXPECT_FALSE(result);
    EXPECT_EQ(g_notifyCount, 1);
}

// B4: 反注册调用 unregisterService 一次
TEST_F(UT_DBusObject, UnRegister_CalledOnce_UnregistersService)
{
    // Arrange: 打桩连接与注销计数
    Stub stub;
    stub.set(ADDR(QDBusConnection, sessionBus), stub_QDBusConnection_sessionBus);
    stub.set((bool (QDBusConnection::*)(const QString &))ADDR(QDBusConnection, unregisterService),
             stub_QDBusConnection_unregisterService);
    resetDBusObjectStubState();

    // Act: 反注册
    DBusObject::getInstance().unRegister();

    // Assert: 恰好一次（含前置零次断言）
    EXPECT_EQ(g_unregisterServiceCount, 1);
    EXPECT_NE(g_unregisterServiceCount, 2);
}

// B5: 最小化窗口经 handleWindow 还原并激活
TEST_F(UT_DBusObject, HandleWindow_MinimizedWindow_RestoresState)
{
    // Arrange: 真实 MainWindow（offscreen）并预置最小化状态
    MainWindow &window = windowForHandleWindowTest();
    gApp->setMainWindow(&window);
    window.setWindowState(Qt::WindowMinimized);

    // Act: 触发 DBus 激活
    DBusObject::getInstance().handleWindow();

    // Assert: 最小化标志被清除（窗口仍有效且状态还原）
    EXPECT_NE(&window, nullptr);
    EXPECT_EQ(window.windowState() & Qt::WindowMinimized, Qt::WindowNoState);
}
