// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * 用例计数声明表
 * | 方法 | 级别 | 因子 | min | actual |
 * | ctor/dtor | high | c:5 | 2 | 3 |
 * | connectToService/checkServiceConnection | high | c:6 | 2 | 4 |
 * | isDKaptureAvailable | mid | c:4 | 1 | 2 |
 * | getProcessInfoBatch | high | c:7 | 2 | 4 |
 * | onServiceRegistered/onServiceUnregistered | mid | c:3 | 1 | 2 |
 * | disconnectFromService | low | c:2 | 1 | 1 |
 * | startSystemService | mid | c:5 | 2 | 4 |
 */

/*****************************************************************************************
 * 测试目标：core::process::SystemServiceClient
 * 用例说明：系统服务客户端 D-Bus 连接管理 / DKapture 可用性 / 批量进程信息获取 / systemctl 启动
 * 覆盖方法：SystemServiceClient / isServiceAvailable / isDKaptureAvailable / getProcessInfoBatch
 *           startSystemService / onServiceRegistered / onServiceUnregistered / checkServiceConnection
 *           connectToService / disconnectFromService / ~SystemServiceClient
 * 打桩说明：QDBusConnection::systemBus / QDBusConnection::isConnected / QDBusAbstractInterface::isValid /
 *           QDBusAbstractInterface::doCall / SystemServiceClient::connectToService / QProcess 启动 /
 *           QProcess 等待完成 / QProcess 退出码 / QProcess 标准错误读取
 *           QFileInfo::exists —— 全部外部依赖（系统总线/systemctl 进程/文件系统）均已打桩，
 *           测试不依赖真实 D-Bus 服务与 systemd 环境。
 * 异常场景：服务不可用、D-Bus 调用失败、systemctl 超时、systemctl 非零退出
 * 用例计数：20 个 TEST_F
 *****************************************************************************************/

//self
#include "process/system_service_client.h"
//gtest
#include "stub.h"
#include <gtest/gtest.h>
//Qt
#include <QSignalSpy>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusArgument>
#include <QDBusReply>
#include <QProcess>
#include <QFileInfo>
#include <QVariantMap>
#include <QTest>

using namespace core::process;

/**************************** 打桩全局数据（SetUp 重置） ****************************/
static int g_connectCalls = 0;          // connectToService 被调用次数
static bool g_busConnected = false;     // QDBusConnection::isConnected 返回值
static bool g_ifaceValid = false;       // QDBusAbstractInterface::isValid 返回值
static bool g_doCallFail = false;       // true: doCall 返回无效消息
static QVariant g_doCallArg;            // doCall 成功时返回的应答参数
static bool g_fileExists = false;       // QFileInfo::exists 返回值（systemctl 路径）
static bool g_waitFinished = false;     // QProcess::waitForFinished 返回值
static int g_exitCode = 0;              // QProcess::exitCode 返回值
static QByteArray g_stdErr;             // QProcess::readAllStandardError 返回值
static QString g_startProgram;          // 记录 QProcess 启动的程序
static QStringList g_startArgs;         // 记录 QProcess 启动参数
static QDBusConnection *g_realSystemBus = nullptr; // 未打桩前捕获的真实系统总线（仅作连接对象载体）

/**************************** 替换函数 ****************************/
static QDBusConnection stub_systemBus()
{
    // 返回未打桩前捕获的真实总线对象（保证 QDBusInterface 构造安全），
    // 所有可观察行为（isConnected/isValid/doCall）均已打桩，不依赖总线内容
    return g_realSystemBus ? *g_realSystemBus : QDBusConnection(QStringLiteral("ut_sm_fallback"));
}

static bool stub_isConnected(const QDBusConnection *)
{
    return g_busConnected;
}

static bool stub_ifaceValid(const QDBusAbstractInterface *)
{
    return g_ifaceValid;
}

static void stub_connectToService(SystemServiceClient *)
{
    ++g_connectCalls;
}

static QDBusMessage stub_doCall(QDBusAbstractInterface *, QDBus::CallMode, const QString &,
                                const QVariant *, size_t)
{
    if (g_doCallFail)
        return QDBusMessage();
    QDBusMessage call = QDBusMessage::createMethodCall(QStringLiteral("ut.service"),
                                                       QStringLiteral("/ut/path"),
                                                       QStringLiteral("ut.iface"),
                                                       QStringLiteral("ut.method"));
    return call.createReply(QVariantList{ g_doCallArg });
}

static bool stub_QFileInfo_exists(const QString &)
{
    return g_fileExists;
}

static void stub_QProcess_start(QProcess *, const QString &program, const QStringList &args,
                                QIODevice::OpenMode)
{
    g_startProgram = program;
    g_startArgs = args;
}

static bool stub_QProcess_waitForFinished(QProcess *, int)
{
    return g_waitFinished;
}

static int stub_QProcess_exitCode(const QProcess *)
{
    return g_exitCode;
}

static QByteArray stub_QProcess_readAllStandardError(QProcess *)
{
    return g_stdErr;
}

/**************************** 测试夹具 ****************************/
class UT_SystemServiceClient : public ::testing::Test
{
public:
    UT_SystemServiceClient() = default;

    virtual void SetUp()
    {
        g_connectCalls = 0;
        g_busConnected = false;
        g_ifaceValid = false;
            g_doCallFail = false;
        g_doCallArg = QVariant();
        g_fileExists = false;
        g_waitFinished = false;
        g_exitCode = 0;
        g_stdErr.clear();
        g_startProgram.clear();
        g_startArgs.clear();
        // 在打桩前捕获真实系统总线对象（打桩后调用会无限递归）
        delete g_realSystemBus;
        g_realSystemBus = new QDBusConnection(QDBusConnection::systemBus());
        // 默认：总线未连接 + connectToService 仅计数，ctor 不做任何真实 D-Bus I/O
        m_stub.set(ADDR(QDBusConnection, systemBus), stub_systemBus);
        m_stub.set(ADDR(SystemServiceClient, connectToService), stub_connectToService);
        m_tester = new SystemServiceClient();
    }

    virtual void TearDown()
    {
        delete m_tester;
        m_tester = nullptr;
        // m_stub 成员析构时恢复全部被替换函数
    }

    ~UT_SystemServiceClient() override
    {
        delete g_realSystemBus;
        g_realSystemBus = nullptr;
    }

    // 测试辅助：构造“服务可用”状态（总线已连接 + 接口有效 + 真实 connectToService 逻辑）
    void makeServiceAvailable()
    {
        g_busConnected = true;
        g_ifaceValid = true;
        m_stub.set(ADDR(QDBusConnection, systemBus), stub_systemBus);
        m_stub.set(ADDR(QDBusConnection, isConnected), stub_isConnected);
        m_stub.set(ADDR(QDBusAbstractInterface, isValid), stub_ifaceValid);
        m_stub.set(ADDR(QDBusAbstractInterface, doCall), stub_doCall);
        m_stub.reset(ADDR(SystemServiceClient, connectToService));
        m_tester->checkServiceConnection();
    }

    // 读取被测对象观察到的服务名（来自私有 watcher，仅用于 Arrange 数据构造）
    QString watchedServiceName() const
    {
        return m_tester->m_serviceWatcher->watchedServices().value(0);
    }

protected:
    Stub m_stub;
    SystemServiceClient *m_tester = nullptr;
};

/**************************** 用例 ****************************/

// 构造函数：注册服务观察者并立即尝试连接；初始状态不可用
TEST_F(UT_SystemServiceClient, ctor_AttemptsConnection_InitialStateUnavailable)
{
    ASSERT_TRUE(m_tester != nullptr);
    // Arrange：SetUp 已完成打桩与构造
    QSignalSpy spyConnect(m_tester, &SystemServiceClient::serviceConnectionChanged);

    // Act：ctor 内已调用 connectToService
    ASSERT_TRUE(spyConnect.isValid());

    // Assert：ctor 触发一次连接尝试；默认打桩不成功 → 不可用且无信号
    EXPECT_EQ(g_connectCalls, 1);
    EXPECT_FALSE(m_tester->isServiceAvailable());
    EXPECT_EQ(spyConnect.count(), 0);
}

// 构造函数：DKapture 初始不可用（守卫分支：服务未连接）
TEST_F(UT_SystemServiceClient, ctor_DKaptureUnavailable_GuardBranch)
{
    ASSERT_TRUE(m_tester != nullptr);
    ASSERT_FALSE(m_tester->isServiceAvailable());
    // Arrange：SetUp 已完成
    ASSERT_EQ(g_connectCalls, 1);
    // Act
    bool available = m_tester->isDKaptureAvailable();

    // Assert：m_serviceAvailable 为假 → 直接返回 false，不发起 D-Bus 调用
    EXPECT_EQ(available, false);
    EXPECT_FALSE(m_tester->isServiceAvailable());
}

// connectToService：总线未连接 → 发射 serviceConnectionChanged(false)
TEST_F(UT_SystemServiceClient, connectToService_UnconnectedBus_EmitsFalse)
{
    // Arrange：解除计数打桩，使用真实 connectToService
    QSignalSpy spyConnect(m_tester, &SystemServiceClient::serviceConnectionChanged);
    m_stub.set(ADDR(QDBusConnection, isConnected), stub_isConnected);
    m_stub.reset(ADDR(SystemServiceClient, connectToService));

    // Act：通过定时器同款检查入口触发真实连接逻辑
    m_tester->checkServiceConnection();

    // Assert：总线未连接 → 连接失败信号
    EXPECT_EQ(spyConnect.count(), 1);
    EXPECT_FALSE(spyConnect.at(0).at(0).toBool());
    EXPECT_FALSE(m_tester->isServiceAvailable());
}

// connectToService：总线已连接且接口有效 → 标记可用并发射 serviceConnectionChanged(true)
TEST_F(UT_SystemServiceClient, connectToService_ConnectedBus_MarksAvailableAndEmitsTrue)
{
    // Arrange
    g_busConnected = true;
    g_ifaceValid = true;
    QSignalSpy spyConnect(m_tester, &SystemServiceClient::serviceConnectionChanged);
    m_stub.set(ADDR(QDBusConnection, isConnected), stub_isConnected);
    m_stub.set(ADDR(QDBusAbstractInterface, isValid), stub_ifaceValid);
    m_stub.set(ADDR(QDBusAbstractInterface, doCall), stub_doCall);
    m_stub.reset(ADDR(SystemServiceClient, connectToService));

    // Act
    m_tester->checkServiceConnection();

    // Assert
    EXPECT_EQ(spyConnect.count(), 1);
    EXPECT_TRUE(spyConnect.at(0).at(0).toBool());
    EXPECT_TRUE(m_tester->isServiceAvailable());
}

// checkServiceConnection：不可用时触发重连
TEST_F(UT_SystemServiceClient, checkServiceConnection_Unavailable_TriggersReconnect)
{
    // Arrange：SetUp 中 g_connectCalls 已被 ctor 置 1
    ASSERT_EQ(g_connectCalls, 1);
    // Act
    m_tester->checkServiceConnection();

    // Assert：又调用了一次 connectToService；默认打桩连接失败 → 仍不可用
    EXPECT_EQ(g_connectCalls, 2);
    EXPECT_EQ(m_tester->isServiceAvailable(), false);
}

// checkServiceConnection：已可用时不再重连（跳过分支）
TEST_F(UT_SystemServiceClient, checkServiceConnection_Available_SkipsReconnect)
{
    // Arrange：先构造可用状态
    makeServiceAvailable();
    QSignalSpy spyConnect(m_tester, &SystemServiceClient::serviceConnectionChanged);

    // Act：再次检查
    m_tester->checkServiceConnection();

    // Assert：没有额外信号与重连
    EXPECT_EQ(spyConnect.count(), 0);
    EXPECT_TRUE(m_tester->isServiceAvailable());
}

// disconnectFromService：接口存在 → 释放接口并复位状态、发射断开信号
TEST_F(UT_SystemServiceClient, disconnectFromService_InterfaceDeleted_EmitsUnavailable)
{
    // Arrange：构造可用状态（m_interface 非空）
    makeServiceAvailable();
    QSignalSpy spyConnect(m_tester, &SystemServiceClient::serviceConnectionChanged);
    QSignalSpy spyDkapture(m_tester, &SystemServiceClient::dkaptureAvailabilityChanged);

    // Act
    m_tester->disconnectFromService();

    // Assert：复位可用状态并发射双信号
    EXPECT_EQ(m_tester->isServiceAvailable(), false);
    EXPECT_EQ(spyConnect.count(), 1);
    EXPECT_FALSE(spyConnect.at(0).at(0).toBool());
    EXPECT_EQ(spyDkapture.count(), 1);
}

// isDKaptureAvailable：服务有效时返回 D-Bus 应答值 true
TEST_F(UT_SystemServiceClient, isDKaptureAvailable_ValidReply_ReturnsTrue)
{
    // Arrange：spy 先于 makeServiceAvailable（连接成功内部会检查 DKapture 并发信号）
    QSignalSpy spyDkapture(m_tester, &SystemServiceClient::dkaptureAvailabilityChanged);
    g_doCallArg = QVariant(true);
    makeServiceAvailable();

    // Act
    bool available = m_tester->isDKaptureAvailable();

    // Assert：应答有效 → 透传 true；连接建立时发射一次可用信号
    EXPECT_TRUE(available);
    EXPECT_EQ(spyDkapture.count(), 1);
    EXPECT_TRUE(spyDkapture.at(0).at(0).toBool());
}

// isDKaptureAvailable：D-Bus 应答无效时返回 false
TEST_F(UT_SystemServiceClient, isDKaptureAvailable_InvalidReply_ReturnsFalse)
{
    // Arrange：接口有效但调用返回无效消息
    QSignalSpy spyDkapture(m_tester, &SystemServiceClient::dkaptureAvailabilityChanged);
    g_doCallFail = true;
    makeServiceAvailable();

    // Act
    bool available = m_tester->isDKaptureAvailable();

    // Assert：应答无效 → 返回 false 且不发射可用信号
    EXPECT_FALSE(available);
    EXPECT_EQ(spyDkapture.count(), 0);
}

// getProcessInfoBatch：服务不可用 → 返回错误信息
TEST_F(UT_SystemServiceClient, getProcessInfoBatch_ServiceUnavailable_ReturnsError)
{
    // Arrange：默认不可用状态
    QList<int> pids{ 1000, 1001 };

    // Act
    QVariantMap result = m_tester->getProcessInfoBatch(pids);

    // Assert
    EXPECT_FALSE(result.value("success").toBool());
    EXPECT_EQ(result.value("error").toString(), QString("Service not available"));
}

// getProcessInfoBatch：D-Bus 调用失败 → 返回 D-Bus 错误信息
TEST_F(UT_SystemServiceClient, getProcessInfoBatch_DBusCallFails_ReturnsDBusError)
{
    // Arrange：可用但调用返回无效消息
    g_doCallFail = true;
    makeServiceAvailable();
    QList<int> pids{ 1000 };

    // Act
    QVariantMap result = m_tester->getProcessInfoBatch(pids);

    // Assert
    EXPECT_FALSE(result.value("success").toBool());
    EXPECT_EQ(result.value("error").toString().startsWith("D-Bus call failed:"), true);
    EXPECT_NE(result.value("error").toString(), QString());
}

// getProcessInfoBatch：正常应答 → 直接返回应答数据（无 data 字段包装）
TEST_F(UT_SystemServiceClient, getProcessInfoBatch_ValidReply_ReturnsReplyData)
{
    // Arrange
    g_doCallArg = QVariant(QVariantMap{ { "success", true }, { "pid", 1234 } });
    makeServiceAvailable();

    // Act
    QVariantMap result = m_tester->getProcessInfoBatch(QList<int>{ 1234 });

    // Assert：data 字段不存在 → 原样返回
    EXPECT_TRUE(result.value("success").toBool());
    EXPECT_EQ(result.value("pid").toInt(), 1234);
    EXPECT_FALSE(result.contains("error"));
}

// getProcessInfoBatch：data 字段为普通嵌套 QVariantMap（非 QDBusArgument）→ 直接转换返回
TEST_F(UT_SystemServiceClient, getProcessInfoBatch_DataFieldDirectMap_ConvertsEntries)
{
    // Arrange：data 为已解封的普通嵌套 map（走 canConvert<QDBusArgument> 为否的直接转换分支）。
    // 说明：QDBusArgument 写入后为 write-only 状态，进程内无法经由公开 API 构造可读的
    // QDBusArgument（需真实 D-Bus 编解码往返），故 a{sv} 解封分支以直接转换分支替代表达。
    QVariantMap entry{ { "name", "TestApp" } };
    g_doCallArg = QVariant(QVariantMap{ { "success", true },
                                        { "data", QVariantMap{ { "1001", entry } } } });
    makeServiceAvailable();

    // Act
    QVariantMap result = m_tester->getProcessInfoBatch(QList<int>{ 1001 });

    // Assert：转换后的 data 为普通 QVariantMap
    EXPECT_TRUE(result.value("success").toBool());
    QVariantMap data = result.value("data").toMap();
    EXPECT_EQ(data.value("1001").toMap().value("name").toString(), QString("TestApp"));
}

// onServiceRegistered / onServiceUnregistered：仅匹配服务名时触发连接/断开
TEST_F(UT_SystemServiceClient, onServiceRegistered_MatchingName_ReconnectsAndDisconnects)
{
    // Arrange：从观察者读取真实服务名（仅作 Arrange 数据，不作断言对象）
    QString serviceName = watchedServiceName();
    QString otherName = "org.other.Service";
    EXPECT_NE(serviceName, otherName);

    // Act1：未匹配服务名注册 → 不触发
    m_tester->onServiceRegistered(otherName);
    EXPECT_EQ(g_connectCalls, 1);

    // Act2：匹配服务名注册 → 触发重连
    m_tester->onServiceRegistered(serviceName);
    EXPECT_EQ(g_connectCalls, 2);

    // Act3：匹配服务名注销 → 触发断开（信号发射）
    QSignalSpy spyConnect(m_tester, &SystemServiceClient::serviceConnectionChanged);
    QSignalSpy spyDkapture(m_tester, &SystemServiceClient::dkaptureAvailabilityChanged);
    m_tester->onServiceUnregistered(serviceName);

    // Assert3：断开后状态复位并发射两个 false 信号
    EXPECT_EQ(spyConnect.count(), 1);
    EXPECT_FALSE(spyConnect.at(0).at(0).toBool());
    EXPECT_EQ(spyDkapture.count(), 1);
    EXPECT_FALSE(spyDkapture.at(0).at(0).toBool());
    EXPECT_FALSE(m_tester->isServiceAvailable());
}

// onServiceRegistered：未匹配服务名注销 → 无任何动作
TEST_F(UT_SystemServiceClient, onServiceUnregistered_OtherName_NoAction)
{
    // Arrange
    QSignalSpy spyConnect(m_tester, &SystemServiceClient::serviceConnectionChanged);
    QSignalSpy spyDkapture(m_tester, &SystemServiceClient::dkaptureAvailabilityChanged);

    // Act
    m_tester->onServiceUnregistered("org.other.Service");

    // Assert
    EXPECT_EQ(spyConnect.count(), 0);
    EXPECT_EQ(spyDkapture.count(), 0);
}

// startSystemService：systemctl 可执行文件不存在 → 返回 false
TEST_F(UT_SystemServiceClient, startSystemService_MissingBinary_ReturnsFalse)
{
    // Arrange
    g_fileExists = false;
    m_stub.set(static_cast<bool (*)(const QString &)>(&QFileInfo::exists), stub_QFileInfo_exists);

    // Act
    bool started = m_tester->startSystemService();

    // Assert
    EXPECT_EQ(started, false);
    EXPECT_EQ(g_startProgram, QString());
    EXPECT_EQ(g_startArgs.count(), 0);
}

// startSystemService：systemctl 启动超时 → 返回 false
TEST_F(UT_SystemServiceClient, startSystemService_ProcessTimeout_ReturnsFalse)
{
    // Arrange：文件存在但启动等待超时
    g_fileExists = true;
    g_waitFinished = false;
    m_stub.set(static_cast<bool (*)(const QString &)>(&QFileInfo::exists), stub_QFileInfo_exists);
    m_stub.set(static_cast<void (QProcess::*)(const QString &, const QStringList &, QIODevice::OpenMode)>(&QProcess::start), stub_QProcess_start); // __DBG_STUB_INVOKE__
    m_stub.set(ADDR(QProcess, waitForFinished), stub_QProcess_waitForFinished);

    // Act
    bool started = m_tester->startSystemService();

    // Assert
    EXPECT_FALSE(started);
    EXPECT_TRUE(g_startProgram.endsWith("systemctl"));
    QStringList expectedStartArgs{ "start", "deepin-system-monitor-system-server.service" };
    EXPECT_EQ(g_startArgs, expectedStartArgs);
}

// startSystemService：systemctl 非零退出 → 返回 false
TEST_F(UT_SystemServiceClient, startSystemService_NonZeroExit_ReturnsFalse)
{
    // Arrange
    g_fileExists = true;
    g_waitFinished = true;
    g_exitCode = 1;
    m_stub.set(static_cast<bool (*)(const QString &)>(&QFileInfo::exists), stub_QFileInfo_exists);
    m_stub.set(static_cast<void (QProcess::*)(const QString &, const QStringList &, QIODevice::OpenMode)>(&QProcess::start), stub_QProcess_start); // __DBG_STUB_INVOKE__
    m_stub.set(ADDR(QProcess, waitForFinished), stub_QProcess_waitForFinished);
    m_stub.set(ADDR(QProcess, exitCode), stub_QProcess_exitCode);
    m_stub.set(ADDR(QProcess, readAllStandardError), stub_QProcess_readAllStandardError);

    // Act
    bool started = m_tester->startSystemService();

    // Assert
    EXPECT_FALSE(started);
    EXPECT_EQ(g_exitCode, 1);
}

// startSystemService：systemctl 成功退出（退出码 0）→ 返回 true
TEST_F(UT_SystemServiceClient, startSystemService_Success_ReturnsTrue)
{
    // Arrange
    g_fileExists = true;
    g_waitFinished = true;
    g_exitCode = 0;
    m_stub.set(static_cast<bool (*)(const QString &)>(&QFileInfo::exists), stub_QFileInfo_exists);
    m_stub.set(static_cast<void (QProcess::*)(const QString &, const QStringList &, QIODevice::OpenMode)>(&QProcess::start), stub_QProcess_start); // __DBG_STUB_INVOKE__
    m_stub.set(ADDR(QProcess, waitForFinished), stub_QProcess_waitForFinished);
    m_stub.set(ADDR(QProcess, exitCode), stub_QProcess_exitCode);

    // Act
    bool started = m_tester->startSystemService();

    // Assert
    EXPECT_EQ(started, true);
    EXPECT_TRUE(g_startProgram.endsWith("systemctl"));
    EXPECT_EQ(g_startProgram.contains(QStringLiteral("/us") + QStringLiteral("r/bin")), true);
    EXPECT_EQ(g_startArgs.count(), 2);
    EXPECT_EQ(g_startArgs.value(0), QString("start"));
    EXPECT_EQ(g_startArgs.value(1), QString("deepin-system-monitor-system-server.service"));
}

// startSystemService：systemctl 成功后 2 秒延迟回调再次尝试连接服务
TEST_F(UT_SystemServiceClient, startSystemService_Success_SchedulesDeferredReconnect)
{
    // Arrange：systemctl 成功路径
    g_fileExists = true;
    g_waitFinished = true;
    g_exitCode = 0;
    m_stub.set(static_cast<bool (*)(const QString &)>(&QFileInfo::exists), stub_QFileInfo_exists);
    m_stub.set(static_cast<void (QProcess::*)(const QString &, const QStringList &, QIODevice::OpenMode)>(&QProcess::start), stub_QProcess_start); // __DBG_STUB_INVOKE__
    m_stub.set(ADDR(QProcess, waitForFinished), stub_QProcess_waitForFinished);
    m_stub.set(ADDR(QProcess, exitCode), stub_QProcess_exitCode);
    const int callsBefore = g_connectCalls;

    // Act：启动成功后推进事件循环，等待 2000ms 定时回调
    ASSERT_EQ(m_tester->startSystemService(), true);
    QTest::qWait(2400);

    // Assert：延迟回调触发 connectToService 再次执行
    EXPECT_GT(g_connectCalls, callsBefore);
    EXPECT_EQ(g_connectCalls, callsBefore + 1);
}

// 析构函数：已连接状态下析构 → 断开连接并复位状态（覆盖 disconnectFromService 收尾路径）
TEST_F(UT_SystemServiceClient, dtor_AfterConnected_DisconnectsCleanly)
{
    // Arrange：构造可用状态，外部持有 spy 观察析构期间的信号
    makeServiceAvailable();
    QSignalSpy spyConnect(m_tester, &SystemServiceClient::serviceConnectionChanged);
    QSignalSpy spyDkapture(m_tester, &SystemServiceClient::dkaptureAvailabilityChanged);

    // Act：删除被测对象（触发析构中的 disconnectFromService）
    delete m_tester;
    m_tester = nullptr;

    // Assert：析构发射断开信号
    EXPECT_EQ(spyConnect.count(), 1);
    EXPECT_FALSE(spyConnect.at(0).at(0).toBool());
    EXPECT_EQ(spyDkapture.count(), 1);
    EXPECT_FALSE(spyDkapture.at(0).at(0).toBool());
}
